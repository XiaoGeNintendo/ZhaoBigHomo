//
// Created by XGN on 2024/6/29.
//

#ifndef ZHAOBIGHOMO_COMPILERTYPES_H
#define ZHAOBIGHOMO_COMPILERTYPES_H
#include <bits/stdc++.h>
using namespace std;

struct Function{
    int startLocation;
    /**
     * Name, Type
     */
    vector<pair<string,string>> parameters;
    string returnType;
};

struct Type{
    Type() = default;
    /**
     * Create a primitive type with the given size. This type has no fields or functions.
     * @param size
     */
    explicit Type(int size){
        this->size=size;
    }
    int size=-1;
    map<string,string> fields;
    map<string,Function> functions;
    int getSize();
};

struct Variable{
    Variable() = default;

    /**
     * Create an int variable with given offset
     * Mainly used for backwards compatability
     * @param offset
     */
    explicit Variable(int offset){
        this->offset=offset;
        this->type="int";
        this->size=1;
    }

    int offset;
    std::string type;
    int size=-1;
    int getSize();
};

#endif //ZHAOBIGHOMO_COMPILERTYPES_H
