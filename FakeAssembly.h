//
// Created by XGN on 2024/6/24.
//

#ifndef ZHAOBIGHOMO_FAKEASSEMBLY_H
#define ZHAOBIGHOMO_FAKEASSEMBLY_H
#include <iostream>

struct Operation{
    int opcode;
    int x,y,z;
    explicit Operation(int opcode=-1, int x=-1, int y=-1, int z=-1):opcode(opcode),x(x),y(y),z(z){}
};

ostream& operator<<(ostream& os, Operation op){
    int t=op.opcode;
    int x=op.x;
    int y=op.y;
    int z=op.z;
    if(t==0){
        os<<"*"<<y<<"="<<x;
    }else if(t==1){
        os<<"*("<<y<<"+*"<<z<<")=*"<<x;
    }else if(t==2){
        os<<"*"<<z<<"="<<"*("<<x<<"+*"<<y<<")";
    }else if(t==3){
        os<<"*"<<y<<"=*"<<x;
    }else if(4<=t && t<=13){
        string w[]={"","","","","+","-","*","/","%","==",">","<","&&","||"};
        os<<"*"<<z<<"=*"<<x<<w[t]<<"*"<<y;
    }else if(t==14){
        os<<"*"<<y<<"=!*"<<x;
    }else if(t==20){
        os<<"If *"<<x<<"!=0 jmp "<<y;
    }else if(t==30){
        os<<"jmp "<<x;
    }else if(t==40){
        os<<"jmp *"<<x;
    }else if(t==50){
        os<<"output *"<<x;
    }else if(t==60){
        os<<"input *"<<x;
    }
    return os;
}

int getOperandCount(int opcode){
    if(opcode==1 || opcode==2 || 4<=opcode && opcode<=13){
        return 3;
    }else if(opcode==0 || opcode==3 || opcode==14 || opcode==20){
        return 2;
    }else if(opcode==30 || opcode==40 || opcode/10==5 || opcode/10==6){
        return 1;
    }
    std::cerr<<"Unknown opcode: "<<opcode<<std::endl;
    exit(2);
}
#endif //ZHAOBIGHOMO_FAKEASSEMBLY_H
