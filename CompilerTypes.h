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
    map<string,string> fields;
    map<string,Function> functions;
    int size();
};

struct Variable{
    int offset;
    std::string type;

    int size();
};

#endif //ZHAOBIGHOMO_COMPILERTYPES_H
