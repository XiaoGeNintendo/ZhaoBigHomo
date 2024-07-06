//
// Created by XGN on 2024/6/24.
//

#include "Expression.h"
#include "FakeAssemblyBuilder.h"


BinaryExpression::~BinaryExpression() {
    delete left;
    delete right;
}

AssignmentExpression::~AssignmentExpression() {
    delete left;
    delete right;
}

FetchAddressExpression::~FetchAddressExpression(){
    delete right;
}


TrinaryExpression::~TrinaryExpression() {
    delete q;
    delete t;
    delete f;
}

string UnaryExpression::compile(vector<Operation> &ops, int putAt) {
    string type=left->compile(ops,putAt+1);
    ops.emplace_back(gGetStack(TEMP,putAt+1));
    if(op.value=="-"){
        ops.emplace_back(gMinus(0,TEMP,TEMP));
    }else if(op.value=="["){
        ops.emplace_back(gArrayGet(TEMP,0,TEMP));
    }else if(op.value=="+") {
        //do nothing
    }else if(op.value=="!"){
        ops.emplace_back(gNot(TEMP,TEMP));
    }else{
        assert(false);
    }
    ops.emplace_back(gSetStack(TEMP,putAt));

    return type;
}

UnaryExpression::~UnaryExpression() {
    delete left;
}

FunctionExpression::~FunctionExpression(){
    for(auto t:parameters){
        delete t;
    }
}