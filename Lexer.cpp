//
// Created by XGN on 2024/6/23.
//
#include "Lexer.h"
//return the next char. fetch a new char
char Lexer::next(){

    while(!(isalpha(fw) || isdigit(fw) || isop(fw) || fw==EOF)){
        fw=getchar();
    }

    char t=fw;
    fw=getchar();
    return t;
}

//return the next char without fetching.
char Lexer::scry(){
    return fw;
}

void Lexer::suckToken(Token t){
    assert(!sucked);
    sucked=true;
    suckedToken=t;
}

Token Lexer::getToken(){
    if(sucked){
        sucked=false;
        return suckedToken;
    }
    string s;
    char c=next();
    if(isop(c)){
        if(c=='=' && scry()=='='){
            next();
            return {OPERATOR,"=="};
        }
        return {OPERATOR,s+c};
    }else if(isdigit(c)){
        s+=c;
        while(isdigit(c=scry())){
            s+=char(next());
        }
        return {INTEGER,s};
    }else if(isalpha(c)){
        s+=c;
        while(isalphaordigit(c=scry())){
            s+=char(next());
        }
        return {IDENTIFIER,s};
    }else if(c==EOF){
        return {ENDOFFILE,""};
    }
}