//
// Created by XGN on 2024/6/23.
//
#ifndef ZHAOBIGHOMO_LEXER_H
#define ZHAOBIGHOMO_LEXER_H

#include <bits/stdc++.h>
using namespace std;
enum TokenType{
    INTEGER,
    IDENTIFIER,
    OPERATOR,
    ENDOFFILE
};

struct Token{
    int type;
    string value;
    Token(){}
    Token(int type, string value):type(type),value(value){}

    bool operator==(const Token& ano) const{
        return type==ano.type && value==ano.value;
    }
};

inline bool isop(int c){
    return c=='+' || c=='-' || c=='*' || c=='/' || c=='='
           || c==';' || c=='?' || c==':' || c=='>' || c=='<' || c=='(' || c==')';
}

inline bool isalphaordigit(int c){
    return isalpha(c) || isdigit(c);
}

class Lexer{
private:
    char next();
    char scry();
    int fw=-2;
    Token suckedToken;
    bool sucked;
    ifstream stream;
public:
    void open(string file);
    Token getToken();
    void suckToken(Token t);
    Token scryToken();
};

const Token EOSToken=Token(OPERATOR,";");
#endif //ZHAOBIGHOMO_LEXER_H
