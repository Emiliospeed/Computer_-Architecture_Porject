#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

int sliced_inst[5];
int i ;





void slice_inst(char * input, int output[]) { // add int sliced_inst[5];

    // Convert hexadecimal string input to integer
    uint32_t instruction = (uint32_t)strtol(input, NULL, 16);

    output[0] = (instruction >> 24) & 0xFF;       // Bits 31:24 (op)
    output[1] = (instruction >> 20) & 0x0F;       // Bits 23:20 (rd)
    output[2] = (instruction >> 16) & 0x0F;       // Bits 19:16 (rs)
    output[3] = (instruction >> 12) & 0x0F;       // Bits 15:12 (rt)
    output[4] = instruction & 0x0FFF;             // Bits 11:0 (imm)



};



void decode(int i){
    //should each of these be done on each attribute {op,rd,rs,rt...?}

    pipe_regs[i][3].pc_pipe = pipe_regs[i][0].pc_pipe;
 
    pipe_regs[i][3].op = pipe_regs[i][0].op;
    pipe_regs[i][3].rd = pipe_regs[i][0].rd;
    pipe_regs[i][3].rs = pipe_regs[i][0].rs;
    pipe_regs[i][3].rt = pipe_regs[i][0].rt;
    pipe_regs[i][3].imm = pipe_regs[i][0].imm;
    pipe_regs[i][3].dist = pipe_regs[i][0].dist;


    //pc_pipe
    pipe_regs[i][3].pc_pipe = pipe_regs[i][0].pc_pipe;
    //inst
    switch (i) {
        case 0:
            for (int k=0;k< LINE_LENGTH ;k++){
                pipe_regs[i][3].inst[k]=Imem0[pc[i]][k];

            }
            break;
        case 1:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][3].inst[k]=Imem1[pc[i]][k];

            }
            break;
        case 2:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][3].inst[k]=Imem2[pc[i]][k];

            }
            break;
        case 3:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][3].inst[k]=Imem3[pc[i]][k];

            }
            break;
    }        
    }
    // op
    //for( int k ; k < LINE_LENGTH ; k++){
    //    sliced_inst( pipe_regs[i][3].inst[k] , sliced_inst[])
    //}
    
    pipe_regs[i][3].op = sliced_inst[0];
    
    //rd
    pipe_regs[i][3].rd = regs[i][sliced_intst[1]];
    //rs
    pipe_regs[i][3].rs = regs[i][sliced_inst[2]];// might need to be converted to decimal from hex
    //rt
    pipe_regs[i][3].rt = regs[i][sliced_inst[3]];// might need to be converted to decimal from hex
    //imm
    pipe_regs[i][3].imm = regs[i][sliced_inst[4]];// might need to be converted to decimal from hex
    //dist 
    pipe_regs[i][3].dist = sliced_intst[1];
    

    //branching 

    if(sliced_inst[0] == 9 || sliced_inst[0]== 10 || sliced_inst[0]==11 || sliced_inst[0]==12 || sliced_inst[0]==13 || sliced_inst[0]==14  ){
        pipe_regs[i][3].pc_pipe =  (regs[i][sliced_inst[4]] & 0x3FF); 

    }
    if (sliced_inst[0] == 15){

        regs[i][15] = pipe_regs[i][0].pc_pipe +1;
        pipe_regs[i][3].pc_pipe =  (regs[i][sliced_inst[4]] & 0x3FF);

    }
        
    
int main() {

    return 2147;
}



// if branch i update int pc[NUMBER_CORES];
//dist is rd index
// convert all to decimal 
//rd contains the contents of rd , not the value of the register but what stored in 
//dalal kikar hamdina
