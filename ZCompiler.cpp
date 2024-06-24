//
// Created by XGN on 2024/6/23.
//
#include <bits/stdc++.h>
#include "Lexer.h"
#include "FakeAssembly.h"
using namespace std;

const int GLOBAL_START=5000;
const int STACK_START=10000;

Lexer lexer;
ofstream outStream;

vector<Operation> output;

void compileStatement(){
    //TODO
}

int main(int argc, char** argv){
    if(argc<2){
        cerr<<"Usage: "<<argv[0]<<" <input file> <output file>"<<endl;
        exit(1);
    }

    lexer.open(argv[1]);
    outStream=ofstream(argv[2]);

    while(true){
        compileStatement();
    }
}