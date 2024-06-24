//
// Created by XGN on 2024/6/23.
//
#include <bits/stdc++.h>
#include "FakeAssembly.h"
using namespace std;

vector<Operation> ops;

int MEM[1000006];
ifstream ins;

inline int read(){
    int x;
    ins>>x;
    return x;
}

int main(int argc, char** argv){

    if(argc<2){
        cerr<<"Usage: "<<argv[0]<<" <input file> <output file>"<<endl;
        return 1;
    }
    if(argc>2){
        freopen(argv[2],"w",stdout);
    }

    ins=ifstream(argv[1]);

    while(!ins.eof()){
        int opcode=read();
        int operandCount=getOperandCount(opcode);
        if(operandCount == 0){
            ops.emplace_back(opcode);
        }else if(operandCount == 1){
            ops.emplace_back(opcode,read());
        }else if(operandCount == 2){
            int x=read();
            int y=read();
            ops.emplace_back(opcode,x,y);
        }else if(operandCount == 3){
            int x=read();
            int y=read();
            int z=read();
            ops.emplace_back(opcode,x,y,z);
        }
    }

    cerr<<"Read Code Succeed. LOC="<<ops.size()<<endl;

    //time to execute
    int pt=0;
    while(true){
        if(pt>=ops.size()){
            break;
        }

        int opcode=ops[pt].opcode;
        int x=ops[pt].x;
        int y=ops[pt].y;
        int z=ops[pt].z;
        switch(opcode){
            case 0:
                MEM[y]=x;
                break;
            case 1:
                MEM[y+MEM[z]]=MEM[x];
                break;
            case 2:
                MEM[z]=MEM[x+MEM[y]];
                break;
            case 3:
                MEM[y]=MEM[x];
                break;
            case 4:
                MEM[z]=MEM[x]+MEM[y];
                break;
            case 5:
                MEM[z]=MEM[x]-MEM[y];
                break;
            case 6:
                MEM[z]=MEM[x]*MEM[y];
                break;
            case 7:
                MEM[z]=MEM[x]/MEM[y];
                break;
            case 8:
                MEM[z]=MEM[x]%MEM[y];
                break;
            case 9:
                MEM[z]=MEM[x]==MEM[y];
                break;
            case 10:
                MEM[z]=MEM[x]>MEM[y];
                break;
            case 11:
                MEM[z]=MEM[x]<MEM[y];
                break;
            case 12:
                MEM[z]=MEM[x]&&MEM[y];
                break;
            case 13:
                MEM[z]=MEM[x]||MEM[y];
                break;
            case 14:
                MEM[y]=!MEM[x];
                break;
            case 20:
                if(MEM[x]){
                    pt=y;
                }else{
                    pt++;
                }
                break;
            case 30:
                pt=x;
                break;
            case 40:
                pt=MEM[x];
                break;
            case 50:
                cout<<MEM[x]<<" ";
                break;
            case 60:
                cin>>MEM[x];
                break;
            default:
                cerr<<"Unrecognized opcode: "<<opcode<<". This should not happen unless you have modified the memory?"<<endl;
                exit(44);
        }

        if(opcode!=20 && opcode!=30 && opcode!=40){
            pt++;
        }
    }

    return 0;
}