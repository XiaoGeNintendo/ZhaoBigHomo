//
// Created by XGN on 2024/6/24.
//

#ifndef ZHAOBIGHOMO_FAKEASSEMBLYBUILDER_H
#define ZHAOBIGHOMO_FAKEASSEMBLYBUILDER_H
#include "FakeAssembly.h"
#include <bits/stdc++.h>
using namespace std;

// Template function to append one vector to another using operator+
template <typename T>
std::vector<T> operator+(const std::vector<T>& vec1, const std::vector<T>& vec2) {
    std::vector<T> result = vec1;  // Start with a copy of the first vector
    result.insert(result.end(), vec2.begin(), vec2.end());  // Append the second vector
    return result;
}


const int GLOBAL_START=5000;
/**
 * MEM[STACK_START] points to the first empty space in stack space
 */
const int STACK_START=10000;
const int STACK_HEAD=STACK_START-1;
const int INPUT_TEMP=4999;
const int OUTPUT_TEMP=4998;
const int LOCAL_INDEX_CACHE=3000;
const int TEMP=100;
const int TEMP_VAR_COUNT=10;

/**
 * Opcode=0 Set the given memory address to the direct value x
 * @param mem
 * @param x
 * @return
 */
Operation gSet(int mem,int x){
    return Operation(0,x,mem);
}

/**
 * Opcode=1. Set MEM[B+MEM[idx]]=MEM[x]
 * @param x
 * @param B
 * @param idx
 * @return
 */
Operation gArraySet(int x,int B, int idx){
    return Operation(1,x,B,idx);
}

/**
 * Opcode=2. Set MEM[x]=MEM[B+MEM[idx]]
 */
Operation gArrayGet(int x,int B,int idx){
 return Operation(2,B,idx,x);
}

/**
 * Opcode=3
 * @param from
 * @param to
 * @return
 */
Operation gCopy(int from, int to){
    return Operation(3,from,to);
}

/**
 * Opcode=4 MEM[to]=MEM[op1]+MEM[op2]
 * @param op1
 * @param op2
 * @param to
 * @return
 */
Operation gAdd(int op1, int op2, int to){
    return Operation(4,op1,op2,to);
}
/**
 * Opcode=5 MEM[to]=MEM[op1]-MEM[op2]
 * @param op1
 * @param op2
 * @param to
 * @return
 */
Operation gMinus(int op1, int op2, int to){
    return Operation(5,op1,op2,to);
}
/**
 * Opcode=14. MEM[to]=!MEM[from]
 */
Operation gNot(int from,int to){
    return Operation(14,from,to);
}

/**
 * Opcode=20 jump to B if MEM[x]=0
 * @param x
 * @param B
 * @return
 */
Operation gJumpIf(int x, int B){
    return Operation(20,x,B);
}

/**
 * Opcode=30
 * @param B
 * @return
 */
Operation gJump(int B){
    return Operation(30,B);
}

/**
 * Opcode=40 jump to MEM[B]
 * @param B
 * @return
 */
Operation gJumpMem(int B){
    return Operation(40,B);
}
/**
 * Opcode=60 reads OUTPUT_TEMP
 * @return
 */
Operation gOutput(){
    return Operation(50,OUTPUT_TEMP);
}

/**
 * Opcode=50 inputs to INPUT_TEMP
 * @return
 */
Operation gInput(){
    return Operation(60,INPUT_TEMP);
}
#endif //ZHAOBIGHOMO_FAKEASSEMBLYBUILDER_H
