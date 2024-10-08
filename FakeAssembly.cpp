//
// Created by XGN on 2024/6/23.
//
#include <bits/stdc++.h>
#include "FakeAssembly.h"
using namespace std;

vector<Operation> ops;

int MEM[1000006];
ifstream ins;

int read(){
    int x;
    ins>>x;
    return x;
}

int main(int argc, char** argv){

    if(argc<2){
        cerr<<"Usage: "<<argv[0]<<" <program file> <output file>"<<endl;
        return 1;
    }
    if(argc>2){
        freopen(argv[2],"w",stdout);
    }

    ins=ifstream(argv[1]);

    int opcode;
    while(ins>>opcode){
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

#ifdef DEBUG_MODE
    cerr<<"Read Code Succeed. LOC="<<ops.size()<<endl;

    int count=0;
    for(auto t:ops){
        cerr<<count<<"\t"<<t<<endl;
        count++;
    }
#endif

    //time to execute
    int pt=0;
    int codeCount=0;
    while(true){
        if(pt>=ops.size()){
            break;
        }

        codeCount++;
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

#ifdef DEBUG_MODE
    cout<<"Total instructions run: "<<codeCount<<endl;
#endif
    return 0;
}