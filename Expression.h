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

     virtual void compileAsLvalue(vector<Operation>& ops, int putAt);
     /**
      * CompilerTypes check the given function and returns the final type
      */
     virtual string typeCheck(){
         return "int";
     }
     virtual ~Expression()=default;
};

class BinaryExpression:public Expression{
public:
    Token op;
    Expression* left;
    Expression* right;

    BinaryExpression(Token op, Expression* left, Expression* right):op(op),left(left),right(right){}
    void compile(vector<Operation> &ops, int putAt) override;
    ~BinaryExpression() override;
};

class ValueExpression: public Expression{
public:
    Token token;
    bool isFunction;
    explicit ValueExpression(Token token,bool isFunction):token(token),isFunction(isFunction){}
    void compile(vector<Operation> &ops, int putAt) override;

    void compileAsLvalue(vector<Operation> &ops, int putAt) override;
};

class UnaryExpression:public Expression{
public:
    Token op;
    Expression* left;

    UnaryExpression(Token op, Expression* left):op(op),left(left){}
    void compile(vector<Operation> &ops, int putAt) override;
    void compileAsLvalue(vector<Operation> &ops, int putAt) override;

    ~UnaryExpression() override;
};

class FetchAddressExpression:public Expression{
public:
    Expression* right;
    explicit FetchAddressExpression(Expression* right):right(right){}
    void compile(vector<Operation> &ops, int putAt) override;
    ~FetchAddressExpression() override;
};

class TrinaryExpression:public Expression{
public:
    Expression* q;
    Expression* t;
    Expression* f;

    TrinaryExpression(Expression* q, Expression* t,Expression* f):q(q),t(t),f(f){}

    void compile(vector<Operation> &ops, int putAt) override;
    ~TrinaryExpression() override;
};

class AssignmentExpression:public Expression{
public:
    Token op;
    Expression* right;
    Expression* left;
    AssignmentExpression(Token op, Expression* left, Expression* right):op(op),left(left),right(right){}
    void compile(vector<Operation> &ops, int putAt) override;
    ~AssignmentExpression() override;
};
struct ExpressionLayerLogic{
    vector<string> operators;
    bool isSpecialLayer;
    function<Expression*(int)> specialOp;
};


#endif //ZHAOBIGHOMO_EXPRESSION_H
