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
inline std::vector<T> operator+(const std::vector<T>& vec1, const std::vector<T>& vec2) {
    std::vector<T> result = vec1;  // Start with a copy of the first vector
    result.insert(result.end(), vec2.begin(), vec2.end());  // Append the second vector
    return result;
}

/**
 * The start of vtable zone
 */
const int VTABLE_START=1000;
/**
 * GLOBAL_START represents the start of global variable zone
 */
const int GLOBAL_START=5000;
/**
 * MEM[STACK_START] points to the first empty space in stack space
 */
const int STACK_START=10000;
const int INPUT_TEMP=4999;
const int OUTPUT_TEMP=4998;
/**
 * [TEMP-10,TEMP+10] is all reserved for registers.
 *
 * So many registers :O
 */
const int TEMP=100;
/**
 * The number of extra stack spaces per layer reserved for storing intermediate values in expressions.
 *
 * Longer expression will bomb.
 */
const int TEMP_VAR_COUNT=50;
/**
 * The location in stack to hold the "this" variable
 */
const int THIS_LOCATION=TEMP_VAR_COUNT+2;

/**
 * Opcode=0 Set the given memory address to the direct value x
 * @param mem
 * @param x
 * @return
 */
inline Operation gSet(int mem,int x){
    return Operation(0,x,mem);
}

/**
 * Opcode=1. Set MEM[B+MEM[idx]]=MEM[x]
 * @param x
 * @param B
 * @param idx
 * @return
 */
inline Operation gArraySet(int x,int B, int idx){
    return Operation(1,x,B,idx);
}

/**
 * Opcode=1. Set MEM[offset+MEM[STACK_START]]=MEM[x]
 * Useful for setting local variables etc.
 */
inline Operation gSetStack(int x,int offset){
    return gArraySet(x,-offset,STACK_START);
}

/**
 * Opcode=2. Set MEM[x]=MEM[B+MEM[idx]]
 */
inline Operation gArrayGet(int x,int B,int idx){
 return Operation(2,B,idx,x);
}

/**
 * Opcode=2. Set MEM[x]=MEM[-offset+MEM[STACK_START]]
 * Useful for fetching local variables etc.
 */
inline Operation gGetStack(int x,int offset){
    return gArrayGet(x,-offset,STACK_START);
}
/**
 * Opcode=3
 * @param from
 * @param to
 * @return
 */
inline Operation gCopy(int from, int to){
    return Operation(3,from,to);
}

/**
 * Opcode=4 MEM[to]=MEM[op1]+MEM[op2]
 * @param op1
 * @param op2
 * @param to
 * @return
 */
inline Operation gAdd(int op1, int op2, int to){
    return Operation(4,op1,op2,to);
}
/**
 * Opcode=5 MEM[to]=MEM[op1]-MEM[op2]
 * @param op1
 * @param op2
 * @param to
 * @return
 */
inline Operation gMinus(int op1, int op2, int to){
    return Operation(5,op1,op2,to);
}

inline Operation gMultiply(int op1, int op2, int to){
    return Operation(6,op1,op2,to);
}
inline Operation gDivide(int op1, int op2, int to){
    return Operation(7,op1,op2,to);
}
inline Operation gMod(int op1, int op2, int to){
    return Operation(8,op1,op2,to);
}
inline Operation gEqual(int op1, int op2, int to){
    return Operation(9,op1,op2,to);
}
inline Operation gGreater(int op1, int op2, int to){
    return Operation(10,op1,op2,to);
}
inline Operation gSmaller(int op1, int op2, int to){
    return Operation(11,op1,op2,to);
}
inline Operation gAnd(int op1, int op2, int to){
    return Operation(12,op1,op2,to);
}
inline Operation gOr(int op1, int op2, int to){
    return Operation(13,op1,op2,to);
}

/**
 * Opcode=14. MEM[to]=!MEM[from]
 */
inline Operation gNot(int from,int to){
    return Operation(14,from,to);
}

/**
 * Opcode=20 jump to B if MEM[x]!=0
 * @param x
 * @param B
 * @return
 */
inline Operation gJumpIf(int x, int B){
    return Operation(20,x,B);
}

/**
 * Opcode=30
 * @param B
 * @return
 */
inline Operation gJump(int B){
    return Operation(30,B);
}

/**
 * Opcode=40 jump to MEM[B]
 * @param B
 * @return
 */
inline Operation gJumpMem(int B){
    return Operation(40,B);
}
/**
 * Opcode=60 reads OUTPUT_TEMP
 * @return
 */
inline Operation gOutput(){
    return Operation(50,OUTPUT_TEMP);
}

/**
 * Opcode=50 inputs to INPUT_TEMP
 * @return
 */
inline Operation gInput(){
    return Operation(60,INPUT_TEMP);
}
#endif //ZHAOBIGHOMO_FAKEASSEMBLYBUILDER_H
