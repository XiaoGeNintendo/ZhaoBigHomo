//
// Created by XGN on 2024/6/24.
//

#include "Expression.h"
#include "FakeAssemblyBuilder.h"

void BinaryExpression::compile(vector<Operation> &ops, int putAt) {

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
        return;
    }

    left->compile(ops,putAt+1);
    right->compile(ops,putAt+2);
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
    }else if(op.value=="!="){
        ops.emplace_back(gEqual(TEMP,TEMP+1,TEMP));
        ops.emplace_back(gNot(TEMP,TEMP));
    }
    ops.emplace_back(gSetStack(TEMP,putAt));
}

BinaryExpression::~BinaryExpression() {
    delete left;
    delete right;
}

AssignmentExpression::~AssignmentExpression() {
    delete right;
}

void TrinaryExpression::compile(vector<Operation> &ops, int putAt) {
    //q
    //if q is true then-|
    //  f               |
    //  jump     -------+--|
    //  t <-------------+  |
    //  copy     <----------
    q->compile(ops,putAt+1);
    ops.emplace_back(gGetStack(TEMP,putAt+1));
    ops.emplace_back(gJumpIf(TEMP,-1));
    int toChange=ops.size()-1;
    //false value
    f->compile(ops,putAt+2);
    ops.emplace_back(gJump(-1));
    int toChange2=ops.size()-1;
    //true value
    ops[toChange].y=ops.size();
    t->compile(ops,putAt+2);
    ops[toChange2].x=ops.size();
    ops.emplace_back(gGetStack(TEMP,putAt+2));
    ops.emplace_back(gSetStack(TEMP,putAt));
}

TrinaryExpression::~TrinaryExpression() {
    delete q;
    delete t;
    delete f;
}

void UnaryExpression::compile(vector<Operation> &ops, int putAt) {
    left->compile(ops,putAt+1);
    ops.emplace_back(gGetStack(TEMP,putAt+1));
    if(op.value=="-"){
        ops.emplace_back(gMinus(0,TEMP,TEMP));
    }
    ops.emplace_back(gSetStack(TEMP,putAt));
}

UnaryExpression::~UnaryExpression() {
    delete left;
}
