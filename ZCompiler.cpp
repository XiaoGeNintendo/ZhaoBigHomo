//
// Created by XGN on 2024/6/23.
//
#include <bits/stdc++.h>
#include "Lexer.h"
#include "FakeAssembly.h"
#include "FakeAssemblyBuilder.h"
#include "Expression.h"
#include "CompilerTypes.h"

using namespace std;

#define EXPECTED_BUT_FOUND 11
#define UNDEFINED_SYMBOL 12
#define DUPLICATE_SYMBOL 13
#define NESTED_FUNCTION 14
#define CUSTOM_FAIL 15
#define KEYWORD_CLASH 16
#define NOT_LVALUE 17
#define TYPE_MISMATCH 18

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
        fail("Expected "+getTokenTypeDisplay(tokenType)+" but found "+getTokenTypeDisplay(token.type),EXPECTED_BUT_FOUND);
    }
}
inline void ensure(const Token& token, const string& tokenValue){
    if(token.value!=tokenValue){
        fail("Expected "+tokenValue+" but found "+token.value,EXPECTED_BUT_FOUND);
    }
}

inline void ensureNext(const string& x){
    ensure(lexer.getToken(),x);
}

inline void ensureSameType(const string& found, const string& expect){
    if(found!=expect){
        fail("Type mismatch: Expected "+expect+" but found "+found,TYPE_MISMATCH);
    }
}

inline void throwUndefined(const string& s,const string& process="unknown"){
    fail("Undefined symbol: "+s+" in process "+process,UNDEFINED_SYMBOL);
}

inline void throwDuplicate(const string& s,const string& process="unknown"){
    fail("Duplicated symbol: "+s+" in "+process,DUPLICATE_SYMBOL);
}

inline void throwNested(const string& s){
    fail("Nested function/class is not allowed right now: "+s,NESTED_FUNCTION);
}

inline void throwKeyword(const string& s) {
    fail("Name " + s + " is reserved and should not be used.",KEYWORD_CLASH);
}

inline void throwNotLvalue(){
    fail("Invalid left-hand side in expression or non-memorable expression.",NOT_LVALUE);
}

/**
 * The main output ASM
 */
vector<Operation> output;

/**
 * The current function that is parsing
 */
string currentFunction;
/**
 * The current class that is parsing
 */
string currentClass;

/**
 * Hack for dots. Force a variable/function to search only in class
 */
bool forceInClassSearch;
/**
 * If globalVars[s]=X, then variable s is stored at GLOBAL_START+X
 */
map<string,Variable> globalVars;
int globalVarSize;

/**
 * The local vars location in the current function. If localVars[s]=X, then the address MEM[STACK_START]-X holds the wanted variable
 *
 * Note that 0 should not occur in this map
 */
map<string,Variable> localVars;
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
map<string,Function> functions;
map<string,Type> types;

/**
 * Return in functions do not know how many local variables are there in the function, therefore they do not know how much
 * the stack pointer should decrease when returned.
 *
 * This holds the temporary return commands that need to set up localVarCountInFunc properly after function terminates.
 */
vector<int> returnPostProcess;

//============================================================Type related thingy

int Type::getSize() {
    if(size!=-1){
        return size;
    }

    return size=CLASS_RESERVED+fields.size();
}

//============================================================Variable related thingy

//nothing here :)

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

string Expression::compileAsLvalue(vector<Operation> &ops, int putAt) {
    throwNotLvalue();
}

string TrinaryExpression::compile(vector<Operation> &ops, int putAt) {
    //q
    //if q is true then-|
    //  f               |
    //  jump     -------+--|
    //  t <-------------+  |
    //  copy     <----------
    string typeQ=q->compile(ops,putAt+1);
    ensureSameType(typeQ,"int");

    ops.emplace_back(gGetStack(TEMP,putAt+1));
    ops.emplace_back(gJumpIf(TEMP,-1));
    int toChange=ops.size()-1;
    //false value
    string typeF=f->compile(ops,putAt+2);
    ops.emplace_back(gJump(-1));
    int toChange2=ops.size()-1;
    //true value
    ops[toChange].y=ops.size();
    string typeT=t->compile(ops,putAt+2);
    ops[toChange2].x=ops.size();
    ops.emplace_back(gGetStack(TEMP,putAt+2));
    ops.emplace_back(gSetStack(TEMP,putAt));

    ensureSameType(typeF,typeT);
    return typeT;
}

string BinaryExpression::compileDot(vector<Operation> &ops, int putAt, bool lvalue) {
    //we will use a switch back hack to process dot
    //by switching "currentClass" and "THIS" to trick the system into we are inside a certain class

    string leftType=left->compile(ops,putAt+1); //compute left address

    //fast fail
    if(leftType=="int"){
        fail("int is not indexable",CUSTOM_FAIL);
    }

    string lastType=currentClass; //store the current class for switching back
    bool lastSearch=forceInClassSearch;

    currentClass=leftType; //hack current class
    forceInClassSearch=true;
    //hack "this"
    ops.emplace_back(gGetStack(TEMP+1,putAt+1));
    ops.emplace_back(gGetStack(TEMP,THIS_LOCATION));
    ops.emplace_back(gSetStack(TEMP,putAt+1));
    ops.emplace_back(gSetStack(TEMP+1,THIS_LOCATION));

    string rightType;
    if(lvalue){
        rightType=right->compileAsLvalue(ops,putAt+2);
    }else {
        rightType = right->compile(ops, putAt + 2);
    }

    //switch back
    ops.emplace_back(gGetStack(TEMP,putAt+1));
    ops.emplace_back(gSetStack(TEMP,THIS_LOCATION));
    currentClass=lastType;
    forceInClassSearch=lastSearch;

    //move the result
    ops.emplace_back(gGetStack(TEMP,putAt+2));
    ops.emplace_back(gSetStack(TEMP,putAt));
    return rightType;
}

string BinaryExpression::compile(vector<Operation> &ops, int putAt) {

    //special judge for && and ||
    if(op.value=="&&"){
        left->compile(ops,putAt+1);
        ops.emplace_back(gGetStack(TEMP,putAt+1));
        ops.emplace_back(gJumpIf(TEMP,ops.size()+3));
        //temp==0
        ops.emplace_back(gSetStack(0,putAt)); //sz+1
        ops.emplace_back(gJump(-1)); //sz+2
        int toChange=ops.size()-1;
        right->compile(ops,putAt+1); //start from sz+3
        ops.emplace_back(gGetStack(TEMP,putAt+1));
        ops.emplace_back(gAnd(TEMP,TEMP,TEMP));
        ops.emplace_back(gSetStack(TEMP,putAt));
        ops[toChange].x=ops.size();
        return "int";
    }
    if(op.value=="||"){
        left->compile(ops,putAt+1);
        ops.emplace_back(gGetStack(TEMP,putAt+1));
        ops.emplace_back(gJumpIf(TEMP,-1));
        int toChange=ops.size()-1;
        //temp==0
        right->compile(ops,putAt+1); //start from sz+3
        ops.emplace_back(gGetStack(TEMP,putAt+1));
        ops.emplace_back(gOr(TEMP,TEMP,TEMP));
        ops.emplace_back(gSetStack(TEMP,putAt));
        ops.emplace_back(gJump(ops.size()+2));

        ops[toChange].y=ops.size();
        //temp==1
        ops.emplace_back(gSetStack(1,putAt)); //sz+1
        return "int";
    }

    if(op.value=="."){
        return compileDot(ops,putAt,false);
    }

    string leftType=left->compile(ops,putAt+1);
    string rightType=right->compile(ops,putAt+2);

    ensureSameType(leftType,"int");
    ensureSameType(rightType,"int");

    ops.emplace_back(gGetStack(TEMP,putAt+1));
    ops.emplace_back(gGetStack(TEMP+1,putAt+2));
    if(op.value=="+"){
        ops.emplace_back(gAdd(TEMP,TEMP+1,TEMP));
    }else if(op.value=="-"){
        ops.emplace_back(gMinus(TEMP,TEMP+1,TEMP));
    }else if(op.value=="*"){
        ops.emplace_back(gMultiply(TEMP,TEMP+1,TEMP));
    }else if(op.value=="/"){
        ops.emplace_back(gDivide(TEMP,TEMP+1,TEMP));
    }else if(op.value=="%"){
        ops.emplace_back(gMod(TEMP,TEMP+1,TEMP));
    }else if(op.value=="=="){
        ops.emplace_back(gEqual(TEMP,TEMP+1,TEMP));
    }else if(op.value==">"){
        ops.emplace_back(gGreater(TEMP,TEMP+1,TEMP));
    }else if(op.value=="<"){
        ops.emplace_back(gSmaller(TEMP,TEMP+1,TEMP));
    }else if(op.value=="&&"){
        ops.emplace_back(gAnd(TEMP,TEMP+1,TEMP));
    }else if(op.value=="||"){
        ops.emplace_back(gOr(TEMP,TEMP+1,TEMP));
    }else if(op.value==">="){
        ops.emplace_back(gSmaller(TEMP,TEMP+1,TEMP));
        ops.emplace_back(gNot(TEMP,TEMP));
    }else if(op.value=="<="){
        ops.emplace_back(gGreater(TEMP,TEMP+1,TEMP));
        ops.emplace_back(gNot(TEMP,TEMP));
    }else if(op.value=="!=") {
        ops.emplace_back(gEqual(TEMP, TEMP + 1, TEMP));
        ops.emplace_back(gNot(TEMP, TEMP));
    }else{
        assert(false);
    }
    ops.emplace_back(gSetStack(TEMP,putAt));

    return "int";
}

//nearly same as compileDot
string BinaryExpression::compileAsLvalue(vector<Operation> &ops, int putAt) {
    if(op.value!="."){
        throwNotLvalue();
    }

    return compileDot(ops,putAt,true);
}

string AssignmentExpression::compile(vector<Operation> &ops, int putAt){
    string rightType=right->compile(ops,putAt+1);

    //TEMP will point to the value we want to set
    string leftType=left->compileAsLvalue(ops,putAt+2);

    ensureSameType(rightType,leftType);

    ops.emplace_back(gGetStack(TEMP,putAt+2));
    //TEMP+1 now holds the right value
    ops.emplace_back(gGetStack(TEMP+1,putAt+1));

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

    return leftType;
}

string FetchAddressExpression::compile(vector<Operation> &ops, int putAt){
    right->compileAsLvalue(ops,putAt);
    return "int";
}

string FunctionExpression::compile(vector<Operation> &ops, int putAt){
    string funcName=call; //history problem

    Function func;
    if(currentClass.empty()) {
        //global function
        if (!functions.count(funcName)) {
            throwUndefined(funcName, "function calling");
        }

        //copy parameter
        func = functions[funcName];
    }else{
        //it's in a class
        if(!types[currentClass].functions.count(funcName)){
            throwUndefined(funcName,"function calling in "+currentClass);
        }

        func=types[currentClass].functions[funcName].first;
    }

    if(parameters.size()!=func.parameters.size()){
        fail("Parameter count does not match.");
    }

    //calculate parameters
    for(int i=0;i<parameters.size();i++){
        string givenType=parameters[i]->compile(ops,putAt+1+i);
        string requiredType=func.parameters[i].second;
        ensureSameType(givenType,requiredType);
    }

    //copy "this"
    ops.emplace_back(gGetStack(TEMP+2,THIS_LOCATION));

    //copy parameters
    int nowAt=3;
    for(int i=0;i<parameters.size();i++){
        ops.emplace_back(gGetStack(TEMP+nowAt,putAt+1+i));
        nowAt++;
    }

    ops.emplace_back(gSet(TEMP,-1)); //set jump back position
    int toChange=ops.size()-1;
    ops.emplace_back(gJump(func.startLocation));
    ops[toChange].x=ops.size();
    ops.emplace_back(gSetStack(TEMP,putAt));

    return func.returnType;
}

string ValueExpression::compile(vector<Operation> &ops, int putAt) {
    if(token.type==INTEGER){
        ops.emplace_back(gSet(TEMP,stoi(token.value)));
        ops.emplace_back(gSetStack(TEMP,putAt));

        return "int";
    }else if(token.type==IDENTIFIER){
        //check for special case: true and false
        if(token.value=="true"){
            ops.emplace_back(gSet(TEMP,1));
            ops.emplace_back(gSetStack(TEMP,putAt));
            return "int";
        }else if(token.value=="false"){
            ops.emplace_back(gSet(TEMP,0));
            ops.emplace_back(gSetStack(TEMP,putAt));
            return "int";
        }

        //just a normal variable
        if (!forceInClassSearch && localVars.count(token.value)) {
            int loc = localVars[token.value].offset;
            ops.emplace_back(gGetStack(TEMP, loc));
            ops.emplace_back(gSetStack(TEMP, putAt));

            return localVars[token.value].type;
        }else if(!currentClass.empty() && types[currentClass].fields.count(token.value)){
            //it's a field
            ops.emplace_back(gGetStack(TEMP,THIS_LOCATION));
            //MEM[TEMP] --> start of object
            ops.emplace_back(gArrayGet(TEMP,types[currentClass].fields[token.value].offset,TEMP));
            ops.emplace_back(gSetStack(TEMP,putAt));

            return types[currentClass].fields[token.value].type;
        } else if (!forceInClassSearch && globalVars.count(token.value)) {
            int loc = globalVars[token.value].offset;
            ops.emplace_back(gSetStack(loc + GLOBAL_START, putAt));
            return globalVars[token.value].type;
        } else {
            throwUndefined(token.value, "value expression");
        }

    }else{
        fail("Expected INTEGER or IDENTIFIER in value expression",EXPECTED_BUT_FOUND);
    }
}

string ValueExpression::compileAsLvalue(vector<Operation> &ops, int putAt) {
    if(token.type!=IDENTIFIER){
        throwNotLvalue();
    }

    string type="";
    if(localVars.count(token.value)) {
        ops.emplace_back(gSet(TEMP, localVars[token.value].offset));
        ops.emplace_back(gMinus(STACK_START, TEMP, TEMP));

        type=localVars[token.value].type;
    }else if(!currentClass.empty() && types[currentClass].fields.count(token.value)){
        ops.emplace_back(gGetStack(TEMP,THIS_LOCATION));
        ops.emplace_back(gSet(TEMP+1,types[currentClass].fields[token.value].offset));
        ops.emplace_back(gAdd(TEMP,TEMP+1,TEMP));
        ops.emplace_back(gSetStack(TEMP,putAt));

        type=types[currentClass].fields[token.value].type;
    }else if(globalVars.count(token.value)){
        ops.emplace_back(gSet(TEMP,GLOBAL_START+globalVars[token.value].offset));

        type=globalVars[token.value].type;
    }else{
        throwUndefined(token.value,"value expression (lvalue)");
    }
    ops.emplace_back(gSetStack(TEMP,putAt));
    return type;
}

string UnaryExpression::compileAsLvalue(vector<Operation> &ops, int putAt) {
    if(op.value!="["){
        throwNotLvalue();
    }
    return left->compile(ops,putAt);
}

vector<ExpressionLayerLogic> logics;

Expression* compileExpression(int layer);

void initExpressionParsingModule(){
    ExpressionLayerLogic layerEqual=ExpressionLayerLogic(); //= += etc
    layerEqual.isSpecialLayer=true;
    layerEqual.specialOp=[](int layer)->Expression*{
        auto first= compileExpression(layer+1);
        auto second=lexer.scryToken().value;
        if(isAnyOfAssignment(second)){

            auto op=lexer.getToken();
            Expression* right=compileExpression(layer);
            Expression* assignment=new AssignmentExpression(op,first,right);
            return assignment;
        }else{
            //oops seems not an assignment layer
            return first;
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

    ExpressionLayerLogic layerAddressFunction=ExpressionLayerLogic(); //layer for address
    layerAddressFunction.isSpecialLayer=true;
    layerAddressFunction.specialOp=[](int layer)->Expression*{
        auto token=lexer.getToken();
        if(token.value=="&"){
            auto ind= compileExpression(layer);
            return new FetchAddressExpression(ind);
        }else{
            lexer.suckToken(token);
            return compileExpression(layer+1);
        }
    };

    ExpressionLayerLogic layerDot=ExpressionLayerLogic();
    layerDot.operators={"."};

    ExpressionLayerLogic layerStuff=ExpressionLayerLogic();
    layerStuff.isSpecialLayer=true;
    layerStuff.specialOp=[](int layer)->Expression* {
        auto token = lexer.getToken();
        if (token.value == "(") {
            auto res = compileExpression(0);
            ensureNext(")");
            return res;
        } else if (token.value == "[") {
            //this is a get memory operation. Wow! Different from C/C++ syntax! Cool!!!
            auto exp = compileExpression(0);
            ensureNext("]");
            return new UnaryExpression(token, exp);
        } else {
            if(lexer.scryToken().value=="("){
                //oh! function calling
                lexer.getToken(); //(
                vector<Expression*> parameters;
                while(true){
                    if(lexer.scryToken().value==")"){
                        lexer.getToken();
                        break;
                    }
                    parameters.emplace_back(compileExpression(0));
                    if(lexer.scryToken().value==","){
                        lexer.getToken();
                    }
                }

                return new FunctionExpression(token.value,parameters);
            }else{
                //regular token
                return new ValueExpression(token);
            }
        }

    };
    logics={layerEqual,layerTrinary,layerLogicalOr,layerLogicalAnd,layerLogical,layerPlusMinus,layerMultipleDivide,layerUnary,layerAddressFunction,layerDot,layerStuff};
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

string compileExpression(){
    auto exp= compileExpression(0);
    string resultType=exp->compile(output,2);
    output.emplace_back(gGetStack(TEMP,2));
    delete exp;
    return resultType;
}

/**
 * This checks whether a variable has the same name as an important reserved word
 * @param name
 */
void checkVariableAvailability(const string& name){
    if(name=="true" || name=="false"){
        throwKeyword(name);
    }
    if(name=="var" || name=="def" || name=="if" || name=="while" || name=="input" || name=="output" || name=="class"){
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
            int relativePosition=localVars[secondToken.value].offset;
            output.emplace_back(gInput());
            output.emplace_back(gSetStack(INPUT_TEMP,relativePosition));
        }else if(globalVars.count(secondToken.value)){
            //it's a global variable
            int position=globalVars[secondToken.value].offset;
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
    }else if(firstToken.value=="while") {
        int startOfLoop = output.size();
        loopHeads.push_back(startOfLoop);
        breakLines.emplace_back();

        ensureNext("(");
        compileExpression();
        ensureNext(")");

        output.emplace_back(gNot(TEMP, TEMP));
        output.emplace_back(gJumpIf(TEMP, -1));
        int toChange = output.size() - 1;

        compileStatement();
        output.emplace_back(gJump(startOfLoop));
        output[toChange].y = output.size();

        //change all break statement
        for (int line: breakLines.back()) {
            output[line].x = output.size();
        }
        breakLines.pop_back();
        loopHeads.pop_back();
    }else if(firstToken.value=="var"){
        Variable var=Variable();
        var.type="int";

        auto varName=lexer.getToken();
        ensure(varName,IDENTIFIER);

        //give it a type
        if(lexer.scryToken().value==":"){
            lexer.getToken();
            auto typeName=lexer.getToken();
            ensure(typeName,IDENTIFIER);

            var.type=typeName.value;
        }

        //Check its availability
        checkVariableAvailability(varName.value);

        if(currentFunction.empty()){
            if(currentClass.empty()) {
                //it's a global variable
                if (globalVars.count(varName.value)) {
                    throwDuplicate(varName.value, "global");
                }

                var.offset = globalVarSize;
                globalVarSize++;

                globalVars[varName.value] = var;
            }else{
                //it's a class field
                if(types[currentClass].fields.count(varName.value)){
                    throwDuplicate(varName.value,"class "+currentClass);
                }

                var.offset = types[currentClass].fields.size();

                types[currentClass].fields[varName.value]=var;
            }
        }else{
            //it's a local variable
            if(localVars.count(varName.value)){
                throwDuplicate(varName.value,currentFunction);
            }

            var.offset=currentTotalLocalVarSize+1;

            currentTotalLocalVarSize++;
            localVars[varName.value]=var;
            maximumTotalLocalVarSize=max(maximumTotalLocalVarSize,currentTotalLocalVarSize);
            if(!localVarStack.empty()) {
                localVarStack.back().push_back(varName.value);
            }

            cout<<"Variable definition: "<<varName.value<<" in "<<currentFunction
            <<" assigned to "<<localVars[varName.value].offset
            <<" Mem usage:"<<currentTotalLocalVarSize<<"/"<<maximumTotalLocalVarSize
            <<" Type:"<<localVars[varName.value].type<<endl;
        }

        //give it a default value
        if(lexer.scryToken().value=="="){
            lexer.getToken();
            compileExpression();
            if(currentFunction==""){
                //global variable
                output.emplace_back(gCopy(TEMP,GLOBAL_START+globalVars[varName.value].offset));
            }else{
                //local variable
                output.emplace_back(gSetStack(TEMP,localVars[varName.value].offset));
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
            currentTotalLocalVarSize--;
            localVars.erase(name);
        }
        localVarStack.pop_back();

    }else if(firstToken.value=="def"){
        Function func=Function();
        func.returnType="int";

        //Read function name
        auto funcName=lexer.getToken();
        ensure(funcName,IDENTIFIER);

        //First check its availability
        checkVariableAvailability(funcName.value);

        if(!currentFunction.empty()){ //check not nested function
            throwNested(funcName.value);
        }
        if(currentClass.empty()) {
            if (functions.count(funcName.value)) {
                throwDuplicate(funcName.value, "functions");
            }
        }else{
            if(types[currentClass].functions.count(funcName.value)){
                throwDuplicate(funcName.value,"functions in "+currentClass);
            }
        }

        //Add parameters
        ensureNext("(");
        while(true){
            if(lexer.scryToken().value==")"){
                lexer.getToken();
                break;
            }
            auto name=lexer.getToken();
            ensure(name,IDENTIFIER);
            string type="int";
            if(lexer.scryToken().value==":"){
                auto typeToken=lexer.getToken();
                ensure(typeToken,IDENTIFIER);
                type=typeToken.value;
            }
            func.parameters.emplace_back(name.value,type);

            if(lexer.scryToken().value==","){
                lexer.getToken();
            }
        }

        //This is about return type
        if(lexer.scryToken().value==":"){
            lexer.getToken();
            auto type=lexer.getToken();
            ensure(type,IDENTIFIER);
            func.returnType=type.value;
        }

        //initialize local variables
        currentFunction=funcName.value;
        localVars.clear();
        returnPostProcess.clear();
        localVars["%RETURN_ADDRESS%"]=Variable(1);
        for(int i=1;i<=TEMP_VAR_COUNT;i++){
            localVars["%TEMP"+to_string(i)+"%"]=Variable(i+1);
        }

        Variable This=Variable();
        This.offset=THIS_LOCATION;
        This.type=(currentClass.empty()?"null":currentClass);
        localVars["this"]=This;
        maximumTotalLocalVarSize=currentTotalLocalVarSize=localVars.size();

        //initialize parameter
        func.parameterTotalSize=0;
        func.parameterOffset=currentTotalLocalVarSize+1;
        for(auto para:func.parameters){
            localVars[para.first]=Variable(currentTotalLocalVarSize+1,para.second);
            currentTotalLocalVarSize++;
            maximumTotalLocalVarSize++;
            func.parameterTotalSize++;
        }

        output.emplace_back(gJump(-1)); //to prevent the code being executed when just defined
        int toFinalChange=output.size()-1;

        //prepare to add the function to list
        func.startLocation=output.size();
        if(currentClass.empty()) {
            functions[currentFunction] = func; //adds to global scope
        }else{
            types[currentClass].functions[currentFunction]={func,-1}; //TODO assign function id
        }

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
        output.emplace_back(gSetStack(TEMP+2,THIS_LOCATION)); //retrieve "this"
        for(int i=0;i<func.parameterTotalSize;i++){ //copy parameters primitively
            output.emplace_back(gSetStack(TEMP+3+i,func.parameterOffset+i));
        }

        output.emplace_back(gJump(backTo)); //jump back

        output[toFinalChange].x=output.size();

        //change all returns
        for(auto loc:returnPostProcess){
            output[loc].x=maximumTotalLocalVarSize;
        }

        currentFunction=""; //restore state
    }else if(firstToken.value=="return") {
        if (lexer.scryToken().value == ";") { //return; <==> return 0;
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
    }else if(firstToken.value=="break") {
        if (loopHeads.empty()) {
            fail("Escaped break outside of a loop");
        }
        breakLines.back().push_back(output.size());
        output.emplace_back(gJump(-1));
        ensureNext(";");
    }else if(firstToken.value=="class"){
        //class :/
        auto name=lexer.getToken();

        //check availability
        ensure(name,IDENTIFIER);
        if(types.count(name.value)){
            throwDuplicate(name.value,"class creation");
        }
        if(!currentClass.empty()){
            throwNested(currentClass);
        }

        currentClass=name.value;
        types[currentClass]=Type();
        //TODO extends superclass

        output.emplace_back(gJump(-1));
        int toFinalChange=output.size()-1; //make sure it's not executed
        compileStatement();
        output[toFinalChange].x=output.size();
        types[currentClass].getSize();

        cout<<"New class: "<<currentClass<<" of size "<<types[currentClass].getSize()<<" defined. Use breakpoint to see detail."<<endl;

        currentClass="";

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

    types["int"]=Type();
    types["null"]=Type();

    lexer.open(argv[1]);
    outStream=ofstream(argv[2]);
    output.emplace_back(gSet(1,1));
    output.emplace_back(gSet(STACK_START,STACK_START+TEMP_VAR_COUNT));
    while(true){
        bool res=compileStatement();
        if(!res){
            break;
        }
    }

    if(!functions.count("main")){
        warn("No main function declared. Will not attempt to call main when start up.");
    }else{
        //call main function
        output.emplace_back(gSet(TEMP,output.size()+2));
        output.emplace_back(gJump(functions["main"].startLocation));
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