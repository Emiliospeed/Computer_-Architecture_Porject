#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LINE_MEMIN 1048576
#define LINE_LENGTH 9
#define MAX_LINES 1024
#define NUMBER_CORES 4
#define NUMBER_BLOCKS 64
#define DSRAM_SIZE 256
#define NUMBER_REGS 16

//char memin[][];
char Imem0[MAX_LINES][LINE_LENGTH];
char Imem1[MAX_LINES][LINE_LENGTH];
char Imem2[MAX_LINES][LINE_LENGTH];
char Imem3[MAX_LINES][LINE_LENGTH];
char memout[MAX_LINE_MEMIN][LINE_LENGTH];
int regs0_old[NUMBER_REGS];
int regs0_new[NUMBER_REGS];
int regs1_old[NUMBER_REGS];
int regs1_new[NUMBER_REGS];
int regs2_old[NUMBER_REGS];
int regs2_new[NUMBER_REGS];
int regs3_old[NUMBER_REGS];
int regs3_new[NUMBER_REGS];
char Dsram0[DSRAM_SIZE][LINE_LENGTH];
char Dsram1[DSRAM_SIZE][LINE_LENGTH];
char Dsram2[DSRAM_SIZE][LINE_LENGTH];
char Dsram3[DSRAM_SIZE][LINE_LENGTH];
char Tsram0[NUMBER_BLOCKS][LINE_LENGTH];
char Tsram1[NUMBER_BLOCKS][LINE_LENGTH];
char Tsram2[NUMBER_BLOCKS][LINE_LENGTH];
char Tsram3[NUMBER_BLOCKS][LINE_LENGTH];
int dirty_array0[NUMBER_BLOCKS]; /*if dirty_array0[i] == 1 then the i'th block in Dsram is dirty */
int dirty_array1[NUMBER_BLOCKS]; 
int dirty_array2[NUMBER_BLOCKS]; 
int dirty_array3[NUMBER_BLOCKS]; 
int pipeline0[5];    /*pipeline0[0] is the pc of the command that is in fetch in core0
                      pipeline0[1] is the pc of the command that is in decode in core0
                      pipeline0[2] is the pc of the command that is in excute in core0
                      pipeline0[3] is the pc of the command that is in mem in core0
                      pipeline0[4] is the pc of the command that is in write back in core0
                      if pipeline0[i] == -1 then there is nothing in it*/
int pipeline1[5];
int pipeline2[5];
int pipeline3[5];
int cycles[NUMBER_CORES];       /* cycles[i] is the number of the current cycle in core[i] */
int instructions[NUMBER_CORES]; /* instructions[i] is the number of the instructions done in core[i] */
int read_hit[NUMBER_CORES];     /* read_hit[i] is the number of read_hit in the Dsram of core[i] */
int write_hit[NUMBER_CORES];    /* write_hit[i] is the number of write_hit in the Dsram of core[i] */
int read_miss[NUMBER_CORES];    /* read_miss[i] is the number of read_miss in the Dsram of core[i] */
int write_miss[NUMBER_CORES];   /* write_miss[i] is the number of write_miss in the Dsram of core[i] */
int decode_stall[NUMBER_CORES]; /* decode_stall[i] is the number of decode stalls in the pipeline of core[i] */
int mem_stall[NUMBER_CORES];    /* mem_stall[i] is the number of mem stalls in the pipeline of core[i] */


void main(int argc, char *argv[]){
    int i, c;
    if (argc != 28){
        printf("invalid input");
        exit(0);
    }
    for(i=0; i<NUMBER_REGS; i++){
        regs0_new[i] = regs0_old[i] = 0;
        regs1_new[i] = regs1_old[i] = 0;
        regs2_new[i] = regs2_old[i] = 0;
        regs3_new[i] = regs3_old[i] = 0;
    }
    //Dsram0[DSRAM_SIZE][LINE_LENGTH]
    for(i=0; i<DSRAM_SIZE; i++){
        for(c=0; c<LINE_LENGTH; c++){
            if (c == (LINE_LENGTH-1)){
                Dsram0[i][c] = '\0';
                Dsram1[i][c] = '\0';
                Dsram2[i][c] = '\0';
                Dsram3[i][c] = '\0';
            }
            else{
                Dsram0[i][c] = '0';
                Dsram1[i][c] = '0';
                Dsram2[i][c] = '0';
                Dsram3[i][c] = '0';
            }
            }
        }
    //Tsram0[NUMBER_BLOCKS][LINE_LENGTH];
    for(i=0; i<NUMBER_BLOCKS; i++){
        for(c=0; c<LINE_LENGTH; c++){
            if (c == (LINE_LENGTH-1)){
                Tsram0[i][c] = '\0';
                Tsram1[i][c] = '\0';
                Tsram2[i][c] = '\0';
                Tsram3[i][c] = '\0';
            }
            else{
                Tsram0[i][c] = '0';
                Tsram1[i][c] = '0';
                Tsram2[i][c] = '0';
                Tsram3[i][c] = '0';
            }
        }
    }
    //dirty_array0[NUMBER_BLOCKS];
    for(i=0; i<NUMBER_BLOCKS; i++){
        dirty_array0[i] = 0;
    }
    //pipeline0[5]
    for(i=0; i<5; i++){
        pipeline0[i] = -1;
        pipeline1[i] = -1;
        pipeline2[i] = -1;
        pipeline3[i] = -1;
    }
    /*
    int cycles[NUMBER_CORES];       
    int instructions[NUMBER_CORES]; 
    int read_hit[NUMBER_CORES];     
    int write_hit[NUMBER_CORES];    
    int read_miss[NUMBER_CORES];    
    int write_miss[NUMBER_CORES];   
    int decode_stall[NUMBER_CORES]; 
    int mem_stall[NUMBER_CORES];    
    int pc[NUMBER_CORES];
    */
    for(i=0; i<NUMBER_CORES;i++){
    cycles[i] = 0;
    instructions[i] = 0;
    read_hit[i] = 0;
    write_hit[i] = 0;
    read_miss[i] = 0;
    write_miss[i] = 0;
    decode_stall[i] = 0;
    mem_stall[i] = 0;
    }
}
