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

inline string getTokenTypeDisplay(int type){
    switch(type){
        case 0:
            return "INTEGER";
        case 1:
            return "IDENTIFIER";
        case 2:
            return "OPERATOR";
        case 3:
            return "ENDOFFILE";
        default:
            return "UNKNOWN";
    }
}

struct Token{
    int type;
    string value;
    Token(){}
    Token(int type, string value):type(type),value(value){}

    bool operator==(const Token& ano) const{
        return type==ano.type && value==ano.value;
    }
};
inline bool isop(int c) {
    static const set<char> operators = {
            '+', '-', '*', '/', '%', '=', ';', '?', ':', '>', '<', '(', ')', '{', '}', '!', '&', '|'
    };
    return operators.find(c) != operators.end();
}

inline bool isAnyOfAssignment(const string& s) {
    static const set<string> operators = {
            "=","+=","-=","*=","/="
    };
    return operators.find(s) != operators.end();
}

inline bool isAnyOfLongLogicalOperation(const string& s){
    static const set<string> operators = {
            "&&","||"
    };
    return operators.find(s) != operators.end();
}

inline bool isAnyOfComparator(const string& s){
    static const set<string> operators = {
            "==","!=",">=","<="
    };
    return operators.find(s) != operators.end();

}
inline bool isLongOperator(const string& s){
    return isAnyOfAssignment(s) || isAnyOfLongLogicalOperation(s) | isAnyOfComparator(s);
}

/**
 * Returns true if c is alpha or _
 * @param c
 * @return
 */
inline bool isalpha_(int c){
    return isalpha(c) || c=='_';
}
/**
 * Returns true if c is alpha or _ or digit
 * @param c
 * @return
 */
inline bool isalpha_ordigit(int c){
    return isalpha_(c) || isdigit(c);
}

class Lexer{
private:
    char next();
    char scry();
    int fw=-2;
    vector<Token> suckedTokens;
    ifstream stream;
public:
    int line;
    void open(string file);
    Token getToken();
    void suckToken(Token t);
    Token scryToken();

    void close();
};

const Token EOSToken=Token(OPERATOR,";");
#endif //ZHAOBIGHOMO_LEXER_H
