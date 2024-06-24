//
// Created by XGN on 2024/6/24.
//

#ifndef ZHAOBIGHOMO_FAKEASSEMBLY_H
#define ZHAOBIGHOMO_FAKEASSEMBLY_H
#include <iostream>

struct Operation{
    int opcode;
    int x,y,z;
    explicit Operation(int opcode=0, int x=0, int y=0, int z=0):opcode(opcode),x(x),y(y),z(z){}
};

int getOperandCount(int opcode){
    if(opcode==1 || opcode==2 || 4<=opcode && opcode<=13){
        return 3;
    }else if(opcode==0 || opcode==3 || opcode==14 || opcode==20){
        return 2;
    }else if(opcode==30 || opcode==40 || opcode==50 || opcode==60){
        return 1;
    }
    std::cerr<<"Unknown opcode: "<<opcode<<std::endl;
    exit(2);
}
#endif //ZHAOBIGHOMO_FAKEASSEMBLY_H
