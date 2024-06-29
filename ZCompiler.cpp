//
// Created by XGN on 2024/6/23.
//
#include <bits/stdc++.h>
#include "Lexer.h"
#include "FakeAssembly.h"
#include "FakeAssemblyBuilder.h"
#include "Expression.h"
using namespace std;

#define EXPECTED_BUT_FOUND 11
#define UNDEFINED_SYMBOL 12
#define DUPLICATE_SYMBOL 13
#define NESTED_FUNCTION 14
#define CUSTOM_FAIL 15
#define KEYWORD_CLASH 16

Lexer lexer;
ofstream outStream;

inline void fail(const string& msg,int err=CUSTOM_FAIL){
    cerr<<"Error: [Line "<<lexer.line<<"] "<<msg<<endl;
    exit(err);
}

inline void warn(const string& msg){
    cerr<<"Warning: [Line "<<lexer.line<<"] "<<msg<<endl;
}

inline void ensure(const Token& token, int tokenType){
    if(token.type!=tokenType){
        cerr<<"Error: [Line "<<lexer.line<<"] Expected "<<getTokenTypeDisplay(tokenType)<<" but found "<<getTokenTypeDisplay(token.type)<<endl;
        exit(EXPECTED_BUT_FOUND);
    }
}
inline void ensure(const Token& token, const string& tokenValue){
    if(token.value!=tokenValue){
        cerr<<"Error: [Line "<<lexer.line<<"] Expected "<<tokenValue<<" but found "<<token.value<<endl;
        exit(EXPECTED_BUT_FOUND);
    }
}

inline void ensureNext(const string& x){
    ensure(lexer.getToken(),x);
}

inline void throwUndefined(const string& s,const string& process="unknown"){
    cerr<<"Error: [Line "<<lexer.line<<"] Undefined variable:"<<s<<" in process "<<process<<endl;
    exit(UNDEFINED_SYMBOL);
}

inline void throwDuplicate(const string& s,const string& process="unknown"){
    cerr<<"Error: [Line "<<lexer.line<<"] Duplicated variable name:"<<s<<" in "<<process<<endl;
    exit(DUPLICATE_SYMBOL);
}

inline void throwNested(const string& s){
    cerr<<"Error: [Line "<<lexer.line<<"] Nested function is not allowed right now:"<<s<<endl;
    exit(NESTED_FUNCTION);
}

inline void throwKeyword(const string& s) {
    cerr << "Error: [Line " << lexer.line << "] Name " << s << " is reserved and should not be used." << endl;
    exit(KEYWORD_CLASH);
}

vector<Operation> output;
map<int,string> modifier;

/*
 * Stack Structure:
 *
 */
string currentFunction;
map<string,int> globalVars;
/**
 * The local vars location in the current function. If localVars[s]=X, then the address MEM[STACK_START]-X holds the wanted variable
 *
 * Note that 0 should not occur in this map
 */
map<string,int> localVars;
/**
 * Holds the size of each local variable.
 */
map<string,int> localVarSize;
int currentTotalLocalVarSize,maximumTotalLocalVarSize;
/**
 * Used as a stack. Each element holds the local variables defined in that block of {}
 */
vector<vector<string>> localVarStack;

/**
 * all breaks in current loop using a stack.
 */
vector<vector<int>> breakLines;

/**
 * Used in continue parsing. Also a stack of loop heads
 */
vector<int> loopHeads;

/**
 * This map points to the start program line that a function starts.
 */
map<string,int> functionPointers;

/**
 * Return in functions do not know how many local variables are there in the function, therefore they do not know how much
 * the stack pointer should decrease when returned.
 *
 * This holds the temporary return commands that need to set up localVarCountInFunc properly after function terminates.
 */
vector<int> returnPostProcess;

//============================================================Expression related thingy
/*
 * Expression priority:
 * = += -= *= /= %=
 * ? :
 * ||
 * &&
 * >= <= == > < !=
 * + -
 * * /
 * unary + -
 *()
 * x=3>5?7:-6*5+4>>3
 * x=3>5?7:-30+4>>3
 */

void AssignmentExpression::compile(vector<Operation> &ops, int putAt){
    right->compile(ops,putAt+1);

    //TEMP will point to the value we want to set
    int flag=0;
    if(localVars.count(left)){ //local variable
        flag=1;
        ops.emplace_back(gCopy(STACK_START,TEMP));
        ops.emplace_back(gSet(TEMP+1,localVars[left]));
        ops.emplace_back(gMinus(TEMP,TEMP+1,TEMP));
    }else if(globalVars.count(left)){
        flag=2;
        ops.emplace_back(gSet(TEMP,globalVars[left]+GLOBAL_START));
    }
    //TEMP+1 now holds the right value
    ops.emplace_back(gGetStack(TEMP+1,putAt+1));

    if(flag==0){
        throwUndefined(left,"assignment expression");
    }

    if(op.value=="="){
        //do nothing
    }else if(op.value=="+="){
        ops.emplace_back(gArrayGet(TEMP+2,0,TEMP));
        ops.emplace_back(gAdd(TEMP+1,TEMP+2,TEMP+1));
        //TEMP+1 now holds the updated value
    }else if(op.value=="-="){
        ops.emplace_back(gArrayGet(TEMP+2,0,TEMP));
        ops.emplace_back(gMinus(TEMP+2,TEMP+1,TEMP+1));
    }else if(op.value=="*="){
        ops.emplace_back(gArrayGet(TEMP+2,0,TEMP));
        ops.emplace_back(gMultiply(TEMP+1,TEMP+2,TEMP+1));
    }else if(op.value=="/="){
        ops.emplace_back(gArrayGet(TEMP+2,0,TEMP));
        ops.emplace_back(gDivide(TEMP+2,TEMP+1,TEMP+1));
    }else if(op.value=="%="){
        ops.emplace_back(gArrayGet(TEMP+2,0,TEMP));
        ops.emplace_back(gMod(TEMP+2,TEMP+1,TEMP+1));
    }else{
        fail("Iternal Error: Unknown assignment operation "+op.value);
    }

    ops.emplace_back(gArraySet(TEMP+1,0,TEMP));
    ops.emplace_back(gSetStack(TEMP+1,putAt));

}

void ValueExpression::compile(vector<Operation> &ops, int putAt) {
    if(token.type==INTEGER){
        if(isFunction){
            warn("Arrr, a mighty fine notion ye be havin' there to be callin' an integer!");
        }
        ops.emplace_back(gSet(TEMP,stoi(token.value)));
        ops.emplace_back(gSetStack(TEMP,putAt));
    }else if(token.type==IDENTIFIER){
        if(isFunction){
            //call a function

            //again special judge in true and false
            if(token.value=="true" || token.value=="false"){
                fail("true/false is not callable.");
            }

            if(!functionPointers.count(token.value)){
                throwUndefined(token.value,"value expression (function calling)");
            }

            int backPos=ops.size()+2;
            ops.emplace_back(gSet(TEMP,backPos));
            ops.emplace_back(gJump(functionPointers[token.value]));
            ops.emplace_back(gSetStack(TEMP,putAt));
        }else {

            //check for special case: true and false
            if(token.value=="true"){
                ops.emplace_back(gSet(TEMP,1));
                ops.emplace_back(gSetStack(TEMP,putAt));
                return;
            }else if(token.value=="false"){
                ops.emplace_back(gSet(TEMP,0));
                ops.emplace_back(gSetStack(TEMP,putAt));
                return;
            }

            //just a normal variable
            if (globalVars.count(token.value)) {
                int loc = globalVars[token.value];
                ops.emplace_back(gSetStack(loc + GLOBAL_START, putAt));
            } else if (localVars.count(token.value)) {
                int loc = localVars[token.value];
                ops.emplace_back(gGetStack(TEMP, loc));
                ops.emplace_back(gSetStack(TEMP, putAt));
            } else {
                throwUndefined(token.value, "value expression");
            }
        }
    }else{
        fail("Expected INTEGER or IDENTIFIER in value expression",EXPECTED_BUT_FOUND);
    }
}

vector<ExpressionLayerLogic> logics;

Expression* compileExpression(int layer);

void initExpressionParsingModule(){
    ExpressionLayerLogic layerEqual=ExpressionLayerLogic(); //= += etc
    layerEqual.isSpecialLayer=true;
    layerEqual.specialOp=[](int layer)->Expression*{
        auto first=lexer.getToken();
        auto second=lexer.scryToken().value;
        if(isAnyOfAssignment(second)){
            ensure(first,IDENTIFIER);
            auto op=lexer.getToken();
            Expression* right= compileExpression(layer);
            Expression* assignment=new AssignmentExpression(op,first.value,right);
            return assignment;
        }else{
            //oops seems not an assignment layer
            lexer.suckToken(first);
            return compileExpression(layer+1);
        }
    };

    ExpressionLayerLogic layerTrinary=ExpressionLayerLogic(); //?:
    layerTrinary.isSpecialLayer=true;
    layerTrinary.specialOp=[](int layer)->Expression*{
        auto first=compileExpression(layer+1);
        if(lexer.scryToken().value=="?"){
            lexer.getToken();
            auto second= compileExpression(layer+1);
            ensureNext(":");
            auto third= compileExpression(layer+1);

            return new TrinaryExpression(first,second,third);
        }else{
            return first;
        }
    };

    ExpressionLayerLogic layerLogicalOr=ExpressionLayerLogic();
    layerLogicalOr.operators={"||"};
    ExpressionLayerLogic layerLogicalAnd=ExpressionLayerLogic();
    layerLogicalAnd.operators={"&&"};
    ExpressionLayerLogic layerLogical=ExpressionLayerLogic();
    layerLogical.operators={">","<",">=","<=","!=","=="};
    ExpressionLayerLogic layerPlusMinus=ExpressionLayerLogic();
    layerPlusMinus.operators={"+","-"};
    ExpressionLayerLogic layerMultipleDivide=ExpressionLayerLogic();
    layerMultipleDivide.operators={"*","/","%"};

    ExpressionLayerLogic layerUnary=ExpressionLayerLogic();
    layerUnary.isSpecialLayer=true;
    layerUnary.specialOp=[](int layer)->Expression*{
        auto first=lexer.scryToken();
        if(first.value=="+" || first.value=="-"){
            lexer.getToken();
            return new UnaryExpression(first, compileExpression(layer));
        }
        return compileExpression(layer+1);
    };

    ExpressionLayerLogic layerStuff=ExpressionLayerLogic();
    layerStuff.isSpecialLayer=true;
    layerStuff.specialOp=[](int layer)->Expression*{
        auto token=lexer.getToken();
        if(token.value=="("){
            auto res= compileExpression(0);
            ensureNext(")");
            return res;
        }else{
            bool function=false;
            if(lexer.scryToken().value=="("){
                //oh! function calling
                //TODO parameter function
                ensureNext("(");
                ensureNext(")");
                function=true;
            }
            return new ValueExpression(token,function);
        }
    };

    logics={layerEqual,layerTrinary,layerLogicalOr,layerLogicalAnd,layerLogical,layerPlusMinus,layerMultipleDivide,layerUnary,layerStuff};
}

Expression* compileExpression(int layer){
    if(layer>=logics.size()){
        fail("Internal Error: Expression logic layer overflow",CUSTOM_FAIL);
    }

    if(logics[layer].isSpecialLayer){
        return logics[layer].specialOp(layer);
    }

    //use ordinary route
    auto e=compileExpression(layer+1);
    vector<pair<Token,Expression*>> exp;
    exp.emplace_back(Token(),e);
    while(true){
        if(find(logics[layer].operators.begin(), logics[layer].operators.end(),lexer.scryToken().value)!=logics[layer].operators.end()){
            auto t=lexer.getToken();
            auto ee= compileExpression(layer+1);
            exp.emplace_back(t,ee);
        }else{
            break;
        }
    }

    Expression* now=e;
    for(int i=1;i<exp.size();i++){
        now=new BinaryExpression(exp[i].first,now,exp[i].second);
    }
    return now;
}

void compileExpression(){
    auto exp= compileExpression(0);
    exp->compile(output,2);
    output.emplace_back(gGetStack(TEMP,2));
    delete exp;
}

/**
 * This checks whether a variable has the same name as an important reserved word
 * @param name
 */
void checkVariableAvailability(const string& name){
    if(name=="true" || name=="false"){
        throwKeyword(name);
    }
    if(name=="var" || name=="def" || name=="if" || name=="while"){
        warn(name+" collides with important keyword. This may cause potential issues.");
    }
}

bool compileStatement(){
    auto firstToken=lexer.getToken();

    if(firstToken.type==ENDOFFILE) {
        return false;
    }else if(firstToken==EOSToken){
        //empty statement
        return true;
    }else if(firstToken.value=="input"){
        auto secondToken=lexer.getToken();
        ensure(secondToken,IDENTIFIER);
        if(localVars.count(secondToken.value)){
            //it's a local variable
            int relativePosition=localVars[secondToken.value];
            output.emplace_back(gInput());
            output.emplace_back(gSetStack(INPUT_TEMP,relativePosition));
        }else if(globalVars.count(secondToken.value)){
            //it's a global variable
            int position=globalVars[secondToken.value];
            output.emplace_back(gInput());
            output.emplace_back(gCopy(INPUT_TEMP,GLOBAL_START+position));
        }else{
            throwUndefined(secondToken.value,"input");
        }
        ensureNext(";");
    }else if(firstToken.value=="output"){
        compileExpression();
        output.emplace_back(gCopy(TEMP,OUTPUT_TEMP));
        output.emplace_back(gOutput());
        ensureNext(";");
    }else if(firstToken.value=="if"){
        ensureNext("(");
        compileExpression();
        ensureNext(")");
        output.emplace_back(gNot(TEMP,TEMP));
        output.emplace_back(gJumpIf(TEMP,-1));
        int toChange=output.size()-1;
        compileStatement();
        output[toChange].y=output.size();
        if(lexer.scryToken().value=="else"){
            lexer.getToken();
            output.emplace_back(gJump(-1));
            output[toChange].y=output.size();
            int toChange2=output.size()-1;
            compileStatement();
            output[toChange2].x=output.size();
        }
    }else if(firstToken.value=="while"){
        int startOfLoop=output.size();
        loopHeads.push_back(startOfLoop);
        breakLines.emplace_back();

        ensureNext("(");
        compileExpression();
        ensureNext(")");

        output.emplace_back(gNot(TEMP,TEMP));
        output.emplace_back(gJumpIf(TEMP,-1));
        int toChange=output.size()-1;


        compileStatement();
        output.emplace_back(gJump(startOfLoop));
        output[toChange].y=output.size();

        //change all break statement
        for(int line:breakLines.back()){
            output[line].x=output.size();
        }
        breakLines.pop_back();
        loopHeads.pop_back();
    }else if(firstToken.value=="var"){
        auto varName=lexer.getToken();
        ensure(varName,IDENTIFIER);

        //First check its availability
        checkVariableAvailability(varName.value);

        if(currentFunction==""){
            //it's a global variable
            if(globalVars.count(varName.value)){
                throwDuplicate(varName.value,"global");
            }

            globalVars[varName.value]=globalVars.size();
        }else{
            //it's a local variable
            if(localVars.count(varName.value)){
                throwDuplicate(varName.value,currentFunction);
            }

            localVars[varName.value]=currentTotalLocalVarSize+1;
            localVarSize[varName.value]=1; //TODO for more types change this value
            currentTotalLocalVarSize+=localVarSize[varName.value];
            maximumTotalLocalVarSize=max(maximumTotalLocalVarSize,currentTotalLocalVarSize);
            if(!localVarStack.empty()) {
                localVarStack.back().push_back(varName.value);
            }
            cout<<"Variable definition: "<<varName.value<<" in "<<currentFunction<<" assigned to "<<localVars[varName.value]<<" Mem usage:"<<currentTotalLocalVarSize<<"/"<<maximumTotalLocalVarSize<<endl;
        }

        //give it a default value
        if(lexer.scryToken().value=="="){
            lexer.getToken();
            compileExpression();
            if(currentFunction==""){
                //global variable
                output.emplace_back(gCopy(TEMP,GLOBAL_START+globalVars[varName.value]));
            }else{
                //local variable
                output.emplace_back(gSetStack(TEMP,localVars[varName.value]));
            }
        }
        ensureNext(";");
    }else if(firstToken.value=="{"){
        //firstly, create a new layer
        localVarStack.emplace_back();

        while(true){
            if(lexer.scryToken().value=="}"){
                lexer.getToken();
                break;
            }
            compileStatement();
        }

        //clear all local variable in this layer
        for(const string& name:localVarStack.back()){
            currentTotalLocalVarSize-=localVarSize[name];
            localVars.erase(name);
            localVarSize.erase(name);
        }
        localVarStack.pop_back();

    }else if(firstToken.value=="def"){

        auto funcName=lexer.getToken();
        ensure(funcName,IDENTIFIER);

        //First check its availability
        checkVariableAvailability(funcName.value);

        if(currentFunction!=""){ //check not nested function
            throwNested(funcName.value);
        }
        if(functionPointers.count(funcName.value)){
            throwDuplicate(funcName.value,"functions");
        }

        ensureNext("(");
        //TODO parameter function
        ensureNext(")");

        //initialize local variables
        currentFunction=funcName.value;
        localVars.clear();
        returnPostProcess.clear();
        localVars["%RETURN_ADDRESS%"]=1;
        for(int i=1;i<=TEMP_VAR_COUNT;i++){
            localVars["%TEMP"+to_string(i)+"%"]=i+1;
        }
        maximumTotalLocalVarSize=currentTotalLocalVarSize=localVars.size();

        output.emplace_back(gJump(-1)); //to prevent the code being executed when just defined
        int toFinalChange=output.size()-1;
        functionPointers[currentFunction]=output.size();
        output.emplace_back(gJump(-1)); //prepare to jump to initial code first
        int toChange=output.size()-1;
        int backTo=output.size();

        compileStatement();


        //make sure the function is properly returned
        //same code as the return below. Change both occurrence!!
        output.emplace_back(gSet(TEMP,0));
        output.emplace_back(gGetStack(TEMP+2,1));
        output.emplace_back(gSet(TEMP+1,maximumTotalLocalVarSize));
        output.emplace_back(gMinus(STACK_START,TEMP+1,STACK_START));
        output.emplace_back(gJumpMem(TEMP+2));

        output[toChange].x=output.size(); //make sure the original code jumps to right place

        //function code here
        //TODO reserved for more function code?
        output.emplace_back(gSet(TEMP+1,maximumTotalLocalVarSize));
        output.emplace_back(gAdd(STACK_START,TEMP+1,STACK_START)); //add stack_start by the memory needed
        output.emplace_back(gSetStack(TEMP,1)); //retrieve the return position
        output.emplace_back(gJump(backTo)); //jump back

        output[toFinalChange].x=output.size();

        //change all returns
        for(auto loc:returnPostProcess){
            output[loc].x=maximumTotalLocalVarSize;
        }

        currentFunction=""; //restore state
    }else if(firstToken.value=="return") {
        if (lexer.scryToken().value == ";") {
            output.emplace_back(gSet(TEMP, 0));
        } else {
            compileExpression();
        }
        //same code as above. Change both occurrence!!
        output.emplace_back(gGetStack(TEMP + 2, 1));
        output.emplace_back(gSet(TEMP + 1, ABSENT_NUMBER));
        returnPostProcess.push_back(output.size() - 1); //need to modify this return command later on
        output.emplace_back(gMinus(STACK_START, TEMP + 1, STACK_START));
        output.emplace_back(gJumpMem(TEMP + 2));

        ensureNext(";");
    }else if(firstToken.value=="continue") {
        if(loopHeads.empty()){
            fail("Escaped continue outside of a loop");
        }

        output.emplace_back(gJump(loopHeads.back()));
        ensureNext(";");
    }else if(firstToken.value=="break"){
        if(loopHeads.empty()){
            fail("Escaped break outside of a loop");
        }
        breakLines.back().push_back(output.size());
        output.emplace_back(gJump(-1));
        ensureNext(";");
    }else{
        //consider as expression
        lexer.suckToken(firstToken);
        compileExpression();
        ensureNext(";");
    }

    return true;
}

int main(int argc, char** argv){
    if(argc<2){
        cerr<<"Usage: "<<argv[0]<<" <input file> <output file>"<<endl;
        exit(1);
    }

    initExpressionParsingModule();

    lexer.open(argv[1]);
    outStream=ofstream(argv[2]);
    output.emplace_back(gSet(1,1));
    output.emplace_back(gSet(STACK_START,STACK_START+50));
    while(true){
        bool res=compileStatement();
        if(!res){
            break;
        }
    }

    if(!functionPointers.count("main")){
        warn("No main function declared. Will not attempt to call main when start up.");
    }else{
        //call main function
        output.emplace_back(gSet(TEMP,output.size()+2));
        output.emplace_back(gJump(functionPointers["main"]));
    }

    int count=0;
    for(auto t:output){
        outStream<<t.opcode<<" ";
        if(t.x!=ABSENT_NUMBER){
            outStream<<t.x<<" ";
            if(t.y!=ABSENT_NUMBER){
                outStream<<t.y<<" ";
                if(t.z!=ABSENT_NUMBER){
                    outStream<<t.z<<" ";
                }
            }
        }
        outStream<<endl;

        cout<<count<<"\t"<<t<<endl;
        count++;
    }
    outStream.close();
    lexer.close();
    cout<<"Compile Success in "<<1.0*clock()/CLOCKS_PER_SEC<<" seconds"<<endl;
}