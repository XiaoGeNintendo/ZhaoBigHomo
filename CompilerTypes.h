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
     * Needs to be manually set!!
     */
    int parameterTotalSize,parameterOffset;
    /**
     * Name, Type
     */
    vector<pair<string,string>> parameters;
    string returnType;

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
    }

    explicit Variable(int offset, std::string type):offset(offset),type(type){}
    int offset;
    std::string type;
};

struct Type{
    /**
     * Manually calculated size. Use getSize() to get size. Do not directly access this.
     */
    int size=-1;
    /**
     * Name, (type,offset)
     */
    map<string,Variable> fields;
    /**
     * Name, (type,offset)
     */
    map<string,pair<Function,int>> functions;

    /**
     * Returns the size needed for allocation of this class
     * @return
     */
    int getSize();
};
#endif //ZHAOBIGHOMO_COMPILERTYPES_H
