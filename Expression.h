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
     *
     * @return the type of the expression
     */
     virtual string compile(vector<Operation>& ops, int putAt)=0;

     virtual string compileAsLvalue(vector<Operation>& ops, int putAt);
     /**
      * CompilerTypes check the given function and returns the final type
      */
     virtual string typeCheck(){
         return "int";
     }
     virtual ~Expression()=default;
};

class BinaryExpression:public Expression{
private:
    string compileDot(vector<Operation> &ops, int putAt, bool lvalue);
public:
    Token op;
    Expression* left;
    Expression* right;

    BinaryExpression(Token op, Expression* left, Expression* right):op(op),left(left),right(right){}
    string compile(vector<Operation> &ops, int putAt) override;
    string compileAsLvalue(vector<Operation> &ops, int putAt) override;
    ~BinaryExpression() override;

};

class ValueExpression: public Expression{
public:
    Token token;
    explicit ValueExpression(Token token):token(token){}
    string compile(vector<Operation> &ops, int putAt) override;

    string compileAsLvalue(vector<Operation> &ops, int putAt) override;
};

class FunctionExpression: public Expression{
public:
    string call;
    vector<Expression*> parameters;
    FunctionExpression(string call, vector<Expression*> parameters):call(call),parameters(parameters){}
    string compile(vector<Operation> &ops, int putAt) override;
    ~FunctionExpression() override;
};


class UnaryExpression:public Expression{
public:
    Token op;
    Expression* left;

    UnaryExpression(Token op, Expression* left):op(op),left(left){}
    string compile(vector<Operation> &ops, int putAt) override;
    string compileAsLvalue(vector<Operation> &ops, int putAt) override;

    ~UnaryExpression() override;
};

class FetchAddressExpression:public Expression{
public:
    Expression* right;
    explicit FetchAddressExpression(Expression* right):right(right){}
    string compile(vector<Operation> &ops, int putAt) override;
    ~FetchAddressExpression() override;
};

class TrinaryExpression:public Expression{
public:
    Expression* q;
    Expression* t;
    Expression* f;

    TrinaryExpression(Expression* q, Expression* t,Expression* f):q(q),t(t),f(f){}

    string compile(vector<Operation> &ops, int putAt) override;
    ~TrinaryExpression() override;
};

class AssignmentExpression:public Expression{
public:
    Token op;
    Expression* right;
    Expression* left;
    AssignmentExpression(Token op, Expression* left, Expression* right):op(op),left(left),right(right){}
    string compile(vector<Operation> &ops, int putAt) override;
    ~AssignmentExpression() override;
};

struct ExpressionLayerLogic{
    vector<string> operators;
    bool isSpecialLayer;
    function<Expression*(int)> specialOp;
};


#endif //ZHAOBIGHOMO_EXPRESSION_H
