//
// Created by XGN on 2024/6/23.
//
#include <bits/stdc++.h>
#include "Lexer.h"
#include "FakeAssembly.h"
#include "FakeAssemblyBuilder.h"

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

/*
 * Stack Structure:
 *
 */
/*
 * Expression priority:
 * ()
 * = += -= *= /= %=
 * -
 * >= <= == > < !=
 * >> <<
 * + -
 * * /
 * ? :
 *
 * 3>5?7:6*5+4>>3
 */
string currentFunction;
map<string,int> globalVars;
map<string,int> localVars;
map<string,int> localVarCountInFunc;

void compileExpression(){
    //TODO
    lexer.getToken();
    output.emplace_back(gSet(TEMP,114514));
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
            output.emplace_back(gArraySet(INPUT_TEMP,0,LOCAL_INDEX_CACHE+relativePosition));
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
                output.emplace_back(gArraySet(TEMP,0,LOCAL_INDEX_CACHE+localVars[varName.value]));
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

        output.emplace_back(gJump(-1));
        int toChange=output.size()-1;
        int backTo=output.size();

        compileStatement();

        localVarCountInFunc[currentFunction]=localVars.size();
        currentFunction=""; //restore state
        output[toChange].x=output.size();
        //time to set up local variables
        output.emplace_back(gCopy(STACK_START,TEMP));
        output.emplace_back(gSet(TEMP+1,LOCAL_INDEX_CACHE));
        for(int i=0;i<localVars.size();i++){
            output.emplace_back(gArraySet(0,0,TEMP)); //set the value to 0
            output.emplace_back(gArraySet(TEMP,0,TEMP+1)); //set local index cache
            output.emplace_back(gAdd(1,TEMP,TEMP));
            output.emplace_back(gAdd(1,STACK_START,STACK_START));
            output.emplace_back(gAdd(1,TEMP+1,TEMP+1));
        }
        output.emplace_back(gJump(backTo));
    }else if(firstToken.value=="return"){
        compileExpression();
        for(int i=0;i<localVarCountInFunc[currentFunction];i++){
            output.emplace_back(gMinus(STACK_START,1,STACK_START));
        }
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