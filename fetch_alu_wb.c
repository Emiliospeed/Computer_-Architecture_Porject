void fetch (int i){ // Takes pc[i] and the instruction of the pc and put pc++ and the instruction in pipe_regs[i][1] .
    int h = pc[i];
    h=h+1;
    pipe_regs[i][1].pc_pipe=h;
    switch (i) {
        case 0:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][1].inst[k]=Imem0[pc[i]][k];

            }
            break;
        case 1:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][1].inst[k]=Imem1[pc[i]][k];

            }
            break;
        case 2:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][1].inst[k]=Imem2[pc[i]][k];

            }
            break;
        case 3:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][1].inst[k]=Imem3[pc[i]][k];

            }
            break;
    }        
}
//helper func for alu that do an arithmetic shift right
int arithmetic_shift_right(int num, int shift)
{
	// Check if the shift amount is greater than or equal to 32
	if (shift >= 32) {
		// If the number's sign bit is set, return all 1s (sign-extended)
		// Otherwise, return all 0s
		return (num & 0x80000000) ? 0xFFFFFFFF : 0;
	}
	else if (num & 0x80000000) {
		// If the number's sign bit is set, perform an arithmetic right shift with sign extension
		return (num >> shift) | ~( 0xFFFFFFFF >> shift);
	}
	else {
		// If the number is positive, perform a logical right shift
		return num >> shift;
	}
}
void alu (int i){ // Takes opcode , rs and rd (from pipe_regs[i][2]) and implement the nine options of the alu instruction according ti the opcode (according to the table given in the project page 4) .
                //And the only change that save is the output of the alu (ALU_PIPE) in pipe_regs[i][5] .
    //int pc_pipetag = pipe_regs[i][2].pc_pipe;
    //char insttag[LINE_LENGTH] = pipe_regs[i][2].inst;
    int optag = pipe_regs[i][2].op;
    //int rdtag = pipe_regs[i][2].rd;
    int rstag = pipe_regs[i][2].rs;
    int rttag = pipe_regs[i][2].rt;
    //int immtag = pipe_regs[i][2].imm;
    //int disttag = pipe_regs[i][2].dist;//the index of the dist register(rd)
    //int ALU_pipetag = pipe_regs[i][2].ALU_pipe;//outp of the alu
    //int datatag = pipe_regs[i][2].data;// data from memory 
    switch (optag){
        case 0:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] + regs[i][rttag];
            break;
        case 1:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] - regs[i][rttag];
            break;
        case 2:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] & regs[i][rttag];
            break;
        case 3:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] | regs[i][rttag];
            break;
        case 4:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] ^ regs[i][rttag];
            break;
        case 5:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] * regs[i][rttag];
            break;
        case 6:
            if(regs[i][rttag]>=32){
                pipe_regs[i][5].ALU_pipe = 0;
            }
            else{
                pipe_regs[i][5].ALU_pipe=regs[i][rstag] << regs[i][rttag];
            }    
            break;
        case 7:
            pipe_regs[i][5].ALU_pipe=arithmetic_shift_right(regs[i][rstag],regs[i][rttag]);
            break;
        case 8:
            if(regs[i][rttag]>=32){
                pipe_regs[i][5].ALU_pipe = 0;
            }
            else{
                pipe_regs[i][5].ALU_pipe=regs[i][rstag] >> regs[i][rttag];
            }    
            break;
    }
}
void wb (int i){ //Takes the opcode , destination , output of alu and data from memory (from pipe_regs[i][6]) and according to the opcode he put the correct result in the destination register .
    //int pc_pipetag = pipe_regs[i][6].pc_pipe;
    //char insttag[LINE_LENGTH] = pipe_regs[i][6].inst;
    int optag = pipe_regs[i][6].op;
    //int rdtag = pipe_regs[i][6].rd;
    //int rstag = pipe_regs[i][6].rs;
    //int rttag = pipe_regs[i][6].rt;
    //int immtag = pipe_regs[i][6].imm;
    int disttag = pipe_regs[i][6].dist;//the index of the dist register(rd)
    int ALU_pipetag = pipe_regs[i][6].ALU_pipe;//outp of the alu
    int datatag = pipe_regs[i][6].data;// data from memory
    if (optag==0 || optag==1 || optag==2 || optag==3 || optag==4 || optag==5 || optag==6 || optag==7 || optag==8 ) {
        regs[i][disttag]=ALU_pipetag;
    }
    else if (optag==16){
        regs[i][disttag]=datatag;
    }

}
