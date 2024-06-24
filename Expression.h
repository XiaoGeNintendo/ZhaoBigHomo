//
// Created by XGN on 2024/6/24.
//

#ifndef ZHAOBIGHOMO_EXPRESSION_H
#define ZHAOBIGHOMO_EXPRESSION_H
#include <bits/stdc++.h>
#include "FakeAssembly.h"
#include "Lexer.h"

using namespace std;

class Expression{
public:
    /**
     * Virtual function to compile the expression calculation logic where the final product should be put at putAt
     *
     * You MUST not use any memory before putAt
     * @param ops
     * @param putAt
     */
     virtual void compile(vector<Operation>& ops, int putAt)=0;

     virtual ~Expression()=default;
};

class BinaryExpression:public Expression{
public:
    Token op;
    Expression* left;
    Expression* right;

    void compile(vector<Operation> &ops, int putAt) override;
    ~BinaryExpression() override;
};

class TrinaryExpression:public Expression{
public:
    Expression* q;
    Expression* t;
    Expression* f;

    TrinaryExpression(Expression* p, Expression* t,Expression* f):q(q),t(t),f(f){}

    void compile(vector<Operation> &ops, int putAt) override;
    ~TrinaryExpression() override;
};

class AssignmentExpression:public Expression{
public:
    Token op;
    Expression* right;
    string left;
    AssignmentExpression(Token op, string left, Expression* right):op(op),left(left),right(right){}
    void compile(vector<Operation> &ops, int putAt) override;
    ~AssignmentExpression() override;
};
struct ExpressionLayerLogic{
    vector<string> operators;
    bool isSpecialLayer;
    bool isNullLayer;
    function<Expression*(int layer,int putAt)> specialOp;
};


#endif //ZHAOBIGHOMO_EXPRESSION_H
