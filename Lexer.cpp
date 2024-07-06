//
// Created by XGN on 2024/6/23.
//
#include "Lexer.h"
//return the next char. fetch a new char
char Lexer::next(){

    while(!(isalpha_(fw) || isdigit(fw) || isop(fw) || fw==EOF)){
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

Token Lexer::_getToken(){
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
    assert(false);
}

Token Lexer::getToken() {
    auto t=_getToken();
    if(t.value=="//"){
        tillNextLine();
        return getToken();
    }else if(t.value=="/*"){
        while(_getToken().value!="*/"){

        }
        return getToken();
    }

    return t;
}

void Lexer::open(string file) {
    stream=ifstream(file);
    this->file=file;
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

void Lexer::tillNextLine() {
    string s;
    getline(stream,s);
    fw=-2;
    line++;
}
