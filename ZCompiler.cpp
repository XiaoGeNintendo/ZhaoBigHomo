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
Lexer lexer;
ofstream outStream;

inline void fail(const string& msg,int err=CUSTOM_FAIL){
    cerr<<"Error: [Line "<<lexer.line<<"] "<<msg<<endl;
    exit(err);
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
map<string,int> localVarCountInFunc;

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
    }

    ops.emplace_back(gArraySet(TEMP+1,0,TEMP));
    ops.emplace_back(gSetStack(TEMP+1,putAt));

    throwUndefined(op.value,"assignment expression");
}

void ValueExpression::compile(vector<Operation> &ops, int putAt) {
    if(token.type==INTEGER){
        ops.emplace_back(gSet(TEMP,stoi(token.value)));
        ops.emplace_back(gSetStack(TEMP,putAt));
    }else if(token.type==IDENTIFIER){
        if(globalVars.count(token.value)){
            int loc=globalVars[token.value];
            ops.emplace_back(gSetStack(loc+GLOBAL_START,putAt));
        }else if(localVars.count(token.value)){
            int loc=localVars[token.value];
            ops.emplace_back(gGetStack(TEMP,loc));
            ops.emplace_back(gSetStack(TEMP,putAt));
        }else{
            throwUndefined(token.value,"value expression");
        }
    }else{
        fail("Expected INTEGER or IDENTIFIER in value expression",EXPECTED_BUT_FOUND);
    }
}

vector<ExpressionLayerLogic> logics;

Expression* compileExpression(int layer, int putAt);

void initExpressionParsingModule(){
    ExpressionLayerLogic layerEqual=ExpressionLayerLogic(); //= += etc
    layerEqual.isSpecialLayer=true;
    layerEqual.specialOp=[](int layer, int putAt)->Expression*{
        auto first=lexer.getToken();
        auto second=lexer.scryToken().value;
        if(isAnyOfAssignment(second)){
            ensure(first,IDENTIFIER);
            auto op=lexer.getToken();
            Expression* right= compileExpression(layer,putAt+1);
            Expression* assignment=new AssignmentExpression(op,first.value,right);
            return assignment;
        }else{
            //oops seems not an assignment layer
            lexer.suckToken(first);
            return compileExpression(layer+1,putAt);
        }
    };

    ExpressionLayerLogic layerTrinary=ExpressionLayerLogic(); //?:
    layerTrinary.isSpecialLayer=true;
    layerTrinary.specialOp=[](int layer, int putAt)->Expression*{
        auto first=compileExpression(layer+1,putAt+1);
        if(lexer.scryToken().value=="?"){
            lexer.getToken();
            auto second= compileExpression(layer+1,putAt+2);
            ensureNext(":");
            auto third= compileExpression(layer+1,putAt+3);

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
    layerMultipleDivide.operators={"*","/"};

    ExpressionLayerLogic layerUnary=ExpressionLayerLogic();
    layerUnary.isSpecialLayer=true;
    layerUnary.specialOp=[](int layer, int putAt)->Expression*{
        auto first=lexer.scryToken();
        if(first.value=="+" || first.value=="-"){
            lexer.getToken();
            return new UnaryExpression(first, compileExpression(layer,putAt+1));
        }
        return compileExpression(layer+1,putAt);
    };

    ExpressionLayerLogic layerStuff=ExpressionLayerLogic();
    layerStuff.isSpecialLayer=true;
    layerStuff.specialOp=[](int layer, int putAt)->Expression*{
        auto token=lexer.getToken();
        if(token.value=="("){
            auto res= compileExpression(0,putAt+1);
            ensureNext(")");
            return res;
        }else{
            return new ValueExpression(token);
        }
    };

    logics={layerEqual,layerTrinary,layerLogicalOr,layerLogicalAnd,layerLogical,layerPlusMinus,layerMultipleDivide,layerUnary,layerStuff};
}

Expression* compileExpression(int layer, int putAt){
    if(layer>=logics.size()){
        fail("Internal Error: Expression logic layer overflow",CUSTOM_FAIL);
    }

    if(logics[layer].isSpecialLayer){
        return logics[layer].specialOp(layer,putAt);
    }

    //use ordinary route
    auto e=compileExpression(layer+1,putAt);
    vector<pair<Token,Expression*>> exp;
    exp.emplace_back(Token(),e);
    while(true){
        if(find(logics[layer].operators.begin(), logics[layer].operators.end(),lexer.scryToken().value)!=logics[layer].operators.end()){
            auto t=lexer.getToken();
            auto ee= compileExpression(layer+1,putAt);
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
    auto exp= compileExpression(0,114514);
    exp->compile(output,2);
    output.emplace_back(gGetStack(TEMP,2));
    delete exp;
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
        ensureNext("(");
        compileExpression();
        ensureNext(")");
        output.emplace_back(gNot(TEMP,TEMP));
        output.emplace_back(gJumpIf(TEMP,-1));
        int toChange=output.size()-1;
        compileStatement();
        output.emplace_back(gJump(startOfLoop));
        output[toChange].y=output.size();
    }else if(firstToken.value=="var"){
        auto varName=lexer.getToken();
        ensure(varName,IDENTIFIER);

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

            localVars[varName.value]=localVars.size()+1;
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
        while(true){
            if(lexer.scryToken().value=="}"){
                lexer.getToken();
                break;
            }
            compileStatement();
        }
    }else if(firstToken.value=="def"){

        auto funcName=lexer.getToken();
        ensure(funcName,IDENTIFIER);

        if(currentFunction!=""){ //check not nested function
            throwNested(funcName.value);
        }

        ensureNext("(");
        //TODO parameter function
        ensureNext(")");

        //initialize local variables
        currentFunction=funcName.value;
        localVars.clear();
        localVars["%RETURN_ADDRESS%"]=1;
        for(int i=1;i<=TEMP_VAR_COUNT;i++){
            localVars["%TEMP"+to_string(i)+"%"]=i+1;
        }

        output.emplace_back(gJump(-1)); //prepare to jump to initial code first
        int toChange=output.size()-1;
        int backTo=output.size();

        compileStatement();

        localVarCountInFunc[currentFunction]=localVars.size();

        //make sure the function is properly returned
        output.emplace_back(gSet(TEMP,0));
        output.emplace_back(gSet(TEMP+1,localVarCountInFunc[currentFunction]));
        output.emplace_back(gMinus(STACK_START,TEMP+1,STACK_START));
        output.emplace_back(gJumpMem(STACK_START));

        output[toChange].x=output.size(); //make sure the original code jumps to right place
        //TODO reserved for more function code?
        output.emplace_back(gSet(TEMP,localVarCountInFunc[currentFunction]));
        output.emplace_back(gAdd(STACK_START,TEMP,STACK_START)); //add stack_start by the memory needed
        output.emplace_back(gJump(backTo)); //jump back
        currentFunction=""; //restore state
    }else if(firstToken.value=="return"){
        if(lexer.scryToken().value==";"){
            output.emplace_back(gSet(TEMP,0));
        }else {
            compileExpression();
        }
        output.emplace_back(gSet(TEMP+1,localVarCountInFunc[currentFunction]));
        output.emplace_back(gMinus(STACK_START,TEMP+1,STACK_START));
        output.emplace_back(gJumpMem(STACK_START));

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

    lexer.open(argv[1]);
    outStream=ofstream(argv[2]);
    output.emplace_back(gSet(1,1));
    output.emplace_back(gSet(STACK_START,STACK_START+1));
    while(true){
        bool res=compileStatement();
        if(!res){
            break;
        }
    }

    int count=0;
    for(auto t:output){
        outStream<<t.opcode<<" ";
        if(t.x!=-1){
            outStream<<t.x<<" ";
            if(t.y!=-1){
                outStream<<t.y<<" ";
                if(t.z!=-1){
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