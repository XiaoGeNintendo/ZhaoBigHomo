//
// Created by XGN on 2024/6/24.
//

#include "Expression.h"
#include "FakeAssemblyBuilder.h"

void BinaryExpression::compile(vector<Operation> &ops, int putAt) {
    left->compile(ops,putAt+1);
    right->compile(ops,putAt+2);
    if(op.value=="+"){
        ops.emplace_back(gAdd(putAt+1,putAt+2,putAt));
    }else if(op.value=="-"){
        ops.emplace_back(gMinus(putAt+1,putAt+2,putAt));
    }else if(op.value=="*"){
        ops.emplace_back(gMultiply(putAt+1,putAt+2,putAt));
    }else if(op.value=="/"){
        ops.emplace_back(gDivide(putAt+1,putAt+2,putAt));
    }else if(op.value=="%"){
        ops.emplace_back(gMod(putAt+1,putAt+2,putAt));
    }else if(op.value=="=="){
        ops.emplace_back(gEqual(putAt+1,putAt+2,putAt));
    }else if(op.value==">"){
        ops.emplace_back(gGreater(putAt+1,putAt+2,putAt));
    }else if(op.value=="<"){
        ops.emplace_back(gSmaller(putAt+1,putAt+2,putAt));
    }else if(op.value=="&&"){
        ops.emplace_back(gAnd(putAt+1,putAt+2,putAt));
    }else if(op.value=="||"){
        ops.emplace_back(gOr(putAt+1,putAt+2,putAt));
    }else if(op.value==">="){
        ops.emplace_back(gSmaller(putAt+1,putAt+2,putAt));
        ops.emplace_back(gNot(putAt,putAt));
    }else if(op.value=="<="){
        ops.emplace_back(gGreater(putAt+1,putAt+2,putAt));
        ops.emplace_back(gNot(putAt,putAt));
    }else if(op.value=="!="){
        ops.emplace_back(gEqual(putAt+1,putAt+2,putAt));
        ops.emplace_back(gNot(putAt,putAt));
    }
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
    ops.emplace_back(gJumpIf(putAt+1,-1));
    int toChange=ops.size()-1;
    //false value
    f->compile(ops,putAt+2);
    ops.emplace_back(gJump(-1));
    int toChange2=ops.size()-1;
    //true value
    ops[toChange].y=ops.size();
    t->compile(ops,putAt+2);
    ops[toChange2].x=ops.size();
    ops.emplace_back(gCopy(putAt+2,putAt));
}

TrinaryExpression::~TrinaryExpression() {
    delete q;
    delete t;
    delete f;
}
