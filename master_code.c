#include <stdio.h> //com
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LINE_MEMIN 1048576
#define LINE_LENGTH 9  //line is a string
#define MAX_LINES 1024
#define NUMBER_CORES 4
#define NUMBER_BLOCKS 64
#define DSRAM_SIZE 256
#define NUMBER_REGS 16

void fetch(int i);
void decode(int i);
void execute(int i);
void mem(int i);
void write_back(int i);

typedef struct {
    int pc_pipe;
    char inst[LINE_LENGTH];
    int op;
    int rd;
    int rs;
    int rt;
    int imm;
    int dist; //the index of the dist register(rd)
    int ALU_pipe; //outp of the alu
    int data; // data from memory 
}temp_reg;

typedef struct {
    int arr[NUMBER_CORES];  // Array to store queue elements
    int front;          // Index of the front of the queue
    int rear;           // Index of the rear of the queue
    int size;           // Current size of the queue
} Queue;

// Function to initialize the queue
void initializeQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

// Function to check if the queue is full
int isFull(Queue* q) {
    return q->size == NUMBER_CORES;
}

// Function to check if the queue is empty
int isEmpty(Queue* q) {
    return q->size == 0;
}

// Function to add an element to the queue (enqueue)
void enqueue(Queue* q, int value) {
    if (isFull(q)) {
    } else {
        // Circularly update rear
        q->rear = (q->rear + 1) % NUMBER_CORES;
        q->arr[q->rear] = value;
        q->size++;
    }
}

// Function to remove an element from the queue (dequeue)
int dequeue(Queue* q) {
    if (isEmpty(q)) {
        return -1;  // Indicating empty queue
    } else {
        int dequeuedValue = q->arr[q->front];
        // Circularly update front
        q->front = (q->front + 1) % NUMBER_CORES;
        q->size--;
        return dequeuedValue;
    }
}

int bus_counter;
int counter_limit;
int flushed[NUMBER_CORES];
char Imem0[MAX_LINES][LINE_LENGTH];
char Imem1[MAX_LINES][LINE_LENGTH];
char Imem2[MAX_LINES][LINE_LENGTH];
char Imem3[MAX_LINES][LINE_LENGTH];
char memout[MAX_LINE_MEMIN][LINE_LENGTH];
int regs[NUMBER_CORES][NUMBER_REGS];
temp_reg pipe_regs[NUMBER_CORES][8]; /*the registers of the pipeline (old and new for all the cores)*/
char Dsram[NUMBER_CORES][DSRAM_SIZE][LINE_LENGTH];
char Tsram[NUMBER_CORES][NUMBER_BLOCKS][15];
//int tag_array0[NUMBER_CORES][NUMBER_CORES];
/*int pipeline[NUMBER_CORES][5];    /*pipeline[i][0] is the pc of the command that is in fetch in core i
                      pipeline[i][1] is the pc of the command that is in decode in core i
                      pipeline[i][2] is the pc of the command that is in excute in core i
                      pipeline[i][3] is the pc of the command that is in mem in core i
                      pipeline[i][4] is the pc of the command that is in write back in core i
                      if pipeline0[i] == -1 then there is nothing in it*/
int cycles[NUMBER_CORES];       /* cycles[i] is the number of the current cycle in core[i] */
int instructions[NUMBER_CORES]; /* instructions[i] is the number of the instructions done in core[i] */
int read_hit[NUMBER_CORES];     /* read_hit[i] is the number of read_hit in the Dsram of core[i] */
int write_hit[NUMBER_CORES];    /* write_hit[i] is the number of write_hit in the Dsram of core[i] */
int read_miss[NUMBER_CORES];    /* read_miss[i] is the number of read_miss in the Dsram of core[i] */
int write_miss[NUMBER_CORES];   /* write_miss[i] is the number of write_miss in the Dsram of core[i] */
int decode_stall[NUMBER_CORES]; /* decode_stall[i] is the number of decode stalls in the pipeline of core[i] */
int mem_stall[NUMBER_CORES];    /* mem_stall[i] is the number of mem stalls in the pipeline of core[i] */
int pc[NUMBER_CORES];

// char* filename,   char ***array,   int*;
void main(int argc, char *argv[]){
    int i, c, k;
    if (argc != 28){
        printf("invalid input");
        exit(0);
    }
    char* imem0_txt = argv[1];
    char* imem1_txt = argv[2];
    char* imem2_txt = argv[3];
    char* imem3_txt = argv[4];
    char* memin_txt = argv[5];
    char* memout_txt = argv[6];
    char* regout0_txt = argv[7];
    char* regout1_txt = argv[8];
    char* regout2_txt = argv[9];
    char* regout3_txt = argv[10];
    char* core0trace_txt = argv[11];
    char* core1trace_txt = argv[12];
    char* core2trace_txt = argv[13];
    char* core3trace_txt = argv[14];
    char* bustrace_txt = argv[15];
    char* dsram0_txt = argv[16];
    char* dsram1_txt = argv[17];
    char* dsram2_txt = argv[18];
    char* dsram3_txt = argv[19];
    char* tsram0_txt = argv[20];
    char* tsram1_txt = argv[21];
    char* tsram2_txt = argv[22];
    char* tsram3_txt = argv[23];
    char* stats0_txt = argv[24];
    char* stats1_txt = argv[25];
    char* stats2_txt = argv[26];
    char* stats3_txt = argv[27];


    for(i=0; i<NUMBER_CORES; i++){              /*fills the pipe_regs with zeroes */
        for(c=0; c<8; c++){
            memset(&pipe_regs[i][c], 0, sizeof(pipe_regs[i][c]));
        }
        for(k=0; k<NUMBER_REGS; k++){           /*fills the regs with zeroes */
            regs[i][k] = 0;
        }
        
        for(c=0; c<DSRAM_SIZE; c++){            /*fills the Dsram last element with with \0 or zero */
            for(k=0; k<LINE_LENGTH; k++){
                if (k == (LINE_LENGTH-1)){
                    Dsram[i][c][k] = '\0';
                }
                else{
                    Dsram[i][c][k] = '0';
                }
            }
        }

        for(c=0; c<NUMBER_BLOCKS; c++){           /*fills the Tsram last element with with \0 or zero */
            for(k=0; k<LINE_LENGTH; k++){
                if (k == (LINE_LENGTH-1)){
                    Tsram[i][c][k] = '\0';
                }
                else{
                    Tsram[i][c][k] = '0';
                }
            }
        }

        /*for(c=0; c<5; c++){
            pipeline[i][c] = -1;
        }*/
        /* values intereact with the MESI */
        cycles[i] = 0;
        instructions[i] = 0;
        read_hit[i] = 0;
        write_hit[i] = 0;
        read_miss[i] = 0;
        write_miss[i] = 0;
        decode_stall[i] = 0;
        mem_stall[i] = 0;
        pc[i] = 0;
        flushed[i] = 0;
    }
    bus_counter = 0;
    counter_limit = 0;

    // here we start after init all the global variables and the file names
}
