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
Lexer lexer;
ofstream outStream;

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
 * unary + -
 * * /
 *()
 * x=3>5?7:-6*5+4>>3
 * x=3>5?7:-30+4>>3
 */

void AssignmentExpression::compile(vector<Operation> &ops, int putAt){
    right->compile(ops,putAt+1);

    int flag=0;
    if(localVars.count(left)){
        flag=1;
        ops.emplace_back(gCopy(STACK_START,putAt+2));
        ops.emplace_back(gSet(putAt+3,localVars[left]+1));
        ops.emplace_back(gMinus(putAt+2,putAt+3,putAt+2)); //putAt+2 is now pos
    }else if(globalVars.count(left)){
        flag=2;
        ops.emplace_back(gSet(putAt+2,globalVars[left]+GLOBAL_START));
    }

    if(flag==0){
        throwUndefined(left,"assignment expression");
    }

    if(op.value=="="){
        ops.emplace_back(gArraySet(putAt+1,0,putAt+2));
        ops.emplace_back(gCopy(putAt+1,putAt));
    }else if(op.value=="+="){
        ops.emplace_back(gArrayGet(putAt+3,0,putAt+2));
        ops.emplace_back(gAdd(putAt+1,putAt+3,putAt+3));
        ops.emplace_back(gArraySet(putAt+3,0,putAt+2));
        ops.emplace_back(gArrayGet(putAt,0,putAt+2));
    }else if(op.value=="-="){
        ops.emplace_back(gArrayGet(putAt+3,0,putAt+2));
        ops.emplace_back(gMinus(putAt+1,putAt+3,putAt+3));
        ops.emplace_back(gArraySet(putAt+3,0,putAt+2));
        ops.emplace_back(gArrayGet(putAt,0,putAt+2));
    }else if(op.value=="*="){
        ops.emplace_back(gArrayGet(putAt+3,0,putAt+2));
        ops.emplace_back(gMultiply(putAt+1,putAt+3,putAt+3));
        ops.emplace_back(gArraySet(putAt+3,0,putAt+2));
        ops.emplace_back(gArrayGet(putAt,0,putAt+2));
    }else if(op.value=="/="){
        ops.emplace_back(gArrayGet(putAt+3,0,putAt+2));
        ops.emplace_back(gDivide(putAt+1,putAt+3,putAt+3));
        ops.emplace_back(gArraySet(putAt+3,0,putAt+2));
        ops.emplace_back(gArrayGet(putAt,0,putAt+2));
    }else if(op.value=="%="){
        ops.emplace_back(gArrayGet(putAt+3,0,putAt+2));
        ops.emplace_back(gMod(putAt+1,putAt+3,putAt+3));
        ops.emplace_back(gArraySet(putAt+3,0,putAt+2));
        ops.emplace_back(gArrayGet(putAt,0,putAt+2));
    }
    throwUndefined(op.value,"assignment expression");
}

ExpressionLayerLogic logics[18];

Expression* compileExpression(int layer, int putAt);

void initExpressionParsingModule(){
    ExpressionLayerLogic l0=ExpressionLayerLogic(); //= += etc
    l0.isSpecialLayer=true;
    l0.specialOp=[](int layer, int putAt)->Expression*{
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

    ExpressionLayerLogic l1=ExpressionLayerLogic(); //?:
    l1.isSpecialLayer=true;
    l1.specialOp=[](int layer, int putAt)->Expression*{
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
}




void compileExpression(){

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
            output.emplace_back(gCopy(STACK_START,TEMP));
            output.emplace_back(gSet(TEMP+1,relativePosition));
            output.emplace_back(gMinus(TEMP,TEMP+1,TEMP));
            output.emplace_back(gArraySet(INPUT_TEMP,0,TEMP));
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

            localVars[varName.value]=localVars.size();
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
                output.emplace_back(gCopy(STACK_START,TEMP+1)); //get stack top
                output.emplace_back(gSet(TEMP+2,localVars[varName.value]+1));
                output.emplace_back(gMinus(TEMP+1,TEMP+2,TEMP+1)); //subtract the index
                output.emplace_back(gArraySet(TEMP,0,TEMP+1)); //set
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
        localVars["%RETURN_ADDRESS%"]=0;
        for(int i=1;i<=TEMP_VAR_COUNT;i++){
            localVars["%TEMP"+to_string(i)+"%"]=i;
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