//
// Created by XGN on 2024/6/23.
//
#include "Lexer.h"
//return the next char. fetch a new char
char Lexer::next(){

    while(!(isalpha(fw) || isdigit(fw) || isop(fw) || fw==EOF)){
        if(fw=='\n'){
            line++;
        }
        fw=stream.get();
    }

    char t=fw;
    fw=stream.get();
    return t;
}

//return the next char without fetching.
char Lexer::scry(){
    return fw;
}

void Lexer::suckToken(Token t){
    suckedTokens.push_back(t);
}

Token Lexer::getToken(){
    if(!suckedTokens.empty()){
        auto t=suckedTokens.back();
        suckedTokens.pop_back();
        return t;
    }
    string s;
    char c=next();
    if(isop(c)){
        string largeOp;
        largeOp+=c;
        largeOp+=scry();
        if(isLongOperator(largeOp)){
            next();
            return {OPERATOR,largeOp};
        }
        return {OPERATOR,s+c};
    }else if(isdigit(c)){
        s+=c;
        while(isdigit(c=scry())){
            s+=char(next());
        }
        return {INTEGER,s};
    }else if(isalpha_(c)){
        s+=c;
        while(isalpha_ordigit(c=scry())){
            s+=char(next());
        }
        return {IDENTIFIER,s};
    }else if(c==EOF){
        return {ENDOFFILE,""};
    }
}

void Lexer::open(string file) {
    stream=ifstream(file);
    line=1;
}

Token Lexer::scryToken() {
    auto t=getToken();
    suckToken(t);
    return t;
}

void Lexer::close() {
    stream.close();
}
