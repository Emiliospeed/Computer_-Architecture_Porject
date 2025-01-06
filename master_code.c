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
#define BusRd 1
#define BusRdX 2
#define Flush 3

void fetch(int core_num);
void decode(int core_num);
void execute(int core_num);
void mem(int core_num);
void write_back(int core_num);
void MESI_bus();

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
char Imem0[MAX_LINES][LINE_LENGTH];
char Imem1[MAX_LINES][LINE_LENGTH];
char Imem2[MAX_LINES][LINE_LENGTH];
char Imem3[MAX_LINES][LINE_LENGTH];
char memout[MAX_LINE_MEMIN][LINE_LENGTH];
int regs[NUMBER_CORES][NUMBER_REGS];
temp_reg pipe_regs[NUMBER_CORES][8]; /*the registers of the pipeline (old and new for all the cores)*/
char Dsram[NUMBER_CORES][DSRAM_SIZE][LINE_LENGTH];
char Tsram[NUMBER_CORES][NUMBER_BLOCKS][15];
int cycles[NUMBER_CORES];       /* cycles[i] is the number of the current cycle in core[i] */
int instructions[NUMBER_CORES]; /* instructions[i] is the number of the instructions done in core[i] */
int read_hit[NUMBER_CORES];     /* read_hit[i] is the number of read_hit in the Dsram of core[i] */
int write_hit[NUMBER_CORES];    /* write_hit[i] is the number of write_hit in the Dsram of core[i] */
int read_miss[NUMBER_CORES];    /* read_miss[i] is the number of read_miss in the Dsram of core[i] */
int write_miss[NUMBER_CORES];   /* write_miss[i] is the number of write_miss in the Dsram of core[i] */
int decode_stall[NUMBER_CORES]; /* decode_stall[i] is the number of decode stalls in the pipeline of core[i] */
int mem_stall[NUMBER_CORES];    /* mem_stall[i] is the number of mem stalls in the pipeline of core[i] */
int pc[NUMBER_CORES];
/*putting in a queues the parameters of the bus*/
Queue core_bus_requests;
Queue bus_origid;
Queue bus_cmd;
Queue bus_address;
int bus_shared[NUMBER_CORES];
int bus_busy;
int core_ready[NUMBER_CORES]; /*core_ready[i] is 1 if the bus gave him the block he wanted*/
int need_flush[NUMBER_CORES]; /*need_flush[i] is 1 if the tag is wrong and we need to flush the block before reading the other block*/
int alredy_enqueued[NUMBER_CORES]; /*alredy_enqueued[i] is 1 if the request is alredy in the round robin*/
int who_flush ; // indicates which core will flush {0,1,2,3,4}
int who_needs; // indicates which core (alternatively the mem) needs the data {0,1,2,3,4}
int sharing_block[NUMBER_CORES];

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


    for(i=0; i<NUMBER_CORES; i++){
        for(c=0; c<8; c++){
            memset(&pipe_regs[i][c], 0, sizeof(pipe_regs[i][c]));
        }
        for(k=0; k<NUMBER_REGS; k++){
            regs[i][k] = 0;
        }
        
        for(c=0; c<DSRAM_SIZE; c++){
            for(k=0; k<LINE_LENGTH; k++){
                if (k == (LINE_LENGTH-1)){
                    Dsram[i][c][k] = '\0';
                }
                else{
                    Dsram[i][c][k] = '0';
                }
            }
        }

        for(c=0; c<NUMBER_BLOCKS; c++){
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

        cycles[i] = 0;
        instructions[i] = 0;
        read_hit[i] = 0;
        write_hit[i] = 0;
        read_miss[i] = 0;
        write_miss[i] = 0;
        decode_stall[i] = 0;
        mem_stall[i] = 0;
        pc[i] = 0;
        core_ready[i] = 1;
        need_flush[i] = 0;
        sharing_block[i] = 0;
        bus_shared[i] = 0;
    }
    bus_counter = 0;
    initializeQueue(&core_bus_requests);
    bus_busy = 0;
    who_flush = 4;
    who_needs = 4; 

    // here we start after init all the global variables and the file names
    // start with if busy then MESI()
}

void mem(int core_num){                                  
    int data = 0;
    int op = pipe_regs[core_num][4].op;
    int address = pipe_regs[core_num][4].ALU_pipe;                    
    int wanted_tag = address / pow(2,8); // assuming 20 bit address   // tag is the first 8 bits address 
    int index = address % 256;
    int block = (index / 4);
    char *tr = Tsram[core_num][block];            //[TAG][MESI] block as in the assignment 
    char *tag = tr+2;                                // to ditch first two 
    char tag_hex[4]; // may need to make it 3
    int tag_num =  hex_to_int(bin_to_hex(tag, tag_hex));                 // same tag but in int
    int mesi_state = -1;
    if(*tr == '0'){
        if(*(tr+1) == '0'){
            mesi_state = 0;
        }
        else{
            mesi_state = 1;
        }
    }
    else{
        if(*(tr+1) == '0'){
            mesi_state = 2;
        }
        else{
            mesi_state = 3;
        }
    }

    if(!alredy_enqueued[core_num]){                    // fills the round robin
        if(op == 16){ //lw
            if (tag_num == wanted_tag){ // tag matched
                if(mesi_state != 0){ // not invalid
                    data = hex_to_int(Dsram[core_num][index]);
                }
                else{ // invalid
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, BusRd);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                }
            }
            else{ // tag is not matched
                if(mesi_state == 0 || mesi_state == 2){//invalid or exclusive
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, BusRd);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                }
                else{ // modified or shared
                    need_flush[core_num] = 1;
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, Flush);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                }
            }

        }

        if(op == 17){ //sw
            if(tag_num == wanted_tag){ //tag matched
                if(mesi_state == 0 || mesi_state == 1){ //invalid or shared
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, BusRdX);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                }
            }
            else{//tag not matched
                if(mesi_state == 0 || mesi_state == 2){//invalid or exclusive
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, BusRdX);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                }
                else{ // modified or shared
                    need_flush[core_num] = 1;
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, Flush);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                }
            }
        }
    }
    if(core_ready[core_num] == 1){
        if(op == 16){ // lw
            data = hex_to_int(Dsram[core_num][index]);
        }
        if(op == 17){ // sw
            int_to_hex(pipe_regs[core_num][4].rd, Dsram[core_num][index]);
        }

        pipe_regs[core_num][7].ALU_pipe = pipe_regs[core_num][4].ALU_pipe;
        pipe_regs[core_num][7].data = data;
        pipe_regs[core_num][7].dist = pipe_regs[core_num][4].dist;
        pipe_regs[core_num][7].imm = pipe_regs[core_num][4].imm;
        strcpy(pipe_regs[core_num][4].inst, pipe_regs[core_num][7].inst);
        pipe_regs[core_num][7].op = pipe_regs[core_num][4].op;
        pipe_regs[core_num][7].pc_pipe = pipe_regs[core_num][4].pc_pipe;
        pipe_regs[core_num][7].rd = pipe_regs[core_num][4].rd;
        pipe_regs[core_num][7].rs = pipe_regs[core_num][4].rs;
        pipe_regs[core_num][7].rt = pipe_regs[core_num][4].rd;
    }
}

int data_in_dsram(int core_num, int address){
    int i;
    int to_ret;
    int block = (address / 4) % NUMBER_BLOCKS;
    int wanted_tag = address / pow(2,8);
    to_ret = 4;
    for(i=0; i<NUMBER_CORES; i++){
        if(i != core_num){
            char *tr = Tsram[core_num][block];
            char *tag = tr + 2;
            char tag_hex[4];
            int tag_num =  hex_to_int(bin_to_hex(tag, tag_hex));
            int mesi_state;
            if(*tr == '0'){
                if(*(tr+1) == '0'){
                    mesi_state = 0;
                    continue;
                }
            }
            if(wanted_tag == tag_num){
                sharing_block[i] = 1;
                to_ret = i;
            }
            
            
        }
    }
    return to_ret;
}

void MESI_bus(){
    static int core_number = -1;
    static int bus_origid = -1;
    static int bus_cmd = -1;
    static int bus_address = -1;
    static int bus_sharing = -1;
    int source_data;
    int i;
    int mesi_state;
    int block_number;
    if(!bus_busy){
        core_number = dequeue(&core_bus_requests);
        bus_origid =  dequeue(&bus_origid);
        bus_cmd = dequeue(&bus_cmd);
        bus_address = dequeue(&bus_address);
        if(core_number == -1){
            return;
        }
        // check if the data is in another dsram , if so the bus_shared = 1
        source_data = data_in_dsram(core_number, bus_address);
        if (source_data != 4){
            bus_sharing = 1;
        }
        if(bus_cmd == BusRd || bus_cmd == BusRdX){
            who_needs = bus_origid;
            for(i=0;i<=3;i++){
                if(bus_origid == i) continue;
                if(sharing_block[i] == 1){
                    block_number = (bus_address/4)%NUMBER_BLOCKS;
                    if((Tsram[i][block_number][0] == '1')  && (Tsram[i][block_number][1] == '1')){ // if the messi_state is modified
                        who_flush = i;
                        if(bus_cmd == BusRd){ 
                            Tsram[i][block_number][0] = '0'; // modified --> shared
                            Tsram[bus_origid][block_number][0] = '0'; // change state to shared of the origid core
                            Tsram[bus_origid][block_number][1] = '1';
                        }
                        if(bus_cmd == BusRdX){
                            Tsram[i][block_number][0] = '0'; // modified --> invalid
                            Tsram[i][block_number][1] = '0';
                            Tsram[bus_origid][block_number][0] = '1';// change state to modified of the origid core
                            Tsram[bus_origid][block_number][1] = '1';
                        }
                    }
                    else{ //bus_shared and not modified then the block is exclusive or shared
                        if(bus_cmd == BusRd){// changing state to shared
                            Tsram[i][block_number][0] = '0'; // exclusive or shared --> shared
                            Tsram[i][block_number][1] = '1';
                            Tsram[bus_origid][block_number][0] = '0'; // change state to shared of the origid core
                            Tsram[bus_origid][block_number][1] = '1';
                        }
                        if(bus_cmd == BusRdX){// changing state to invalid
                            Tsram[i][block_number][0] = '0';// exclusive or shared --> invalid
                            Tsram[i][block_number][1] = '0';
                            Tsram[bus_origid][block_number][0] = '1';// change state to modified of the origid core
                            Tsram[bus_origid][block_number][1] = '1';
                        }
                    }
                }
                
            }
            bus_origid = who_flush;
            bus_cmd = Flush;
            bus_busy = 1;
            return;
        }
        
        
    }
    bus_address = (bus_address >> 2) << 2;
    block_number = (bus_address/4)%NUMBER_BLOCKS;
    if(bus_cmd == Flush){
        bus_busy = 1;
        who_flush = bus_origid;
        bus_counter++;
        if(bus_counter>0 && bus_counter<5){
            if(bus_sharing == 1){
                strcpy(Dsram[who_needs][bus_address + bus_counter - 1], Dsram[who_flush][bus_address + bus_counter - 1]);
            }
        }
        if(bus_counter >= 16){
            if((!bus_sharing == 1) && who_needs != 4){
                strcpy(Dsram[who_needs][bus_address + bus_counter - 16], Dsram[who_flush][bus_address + bus_counter - 16]);
            }
            if(who_flush != 4){
                strcpy(memout[bus_address + bus_counter - 16], Dsram[who_flush][bus_address  + bus_counter - 16]);
            }
            if(bus_counter == 19){
                if(who_needs != 4){
                    core_ready[who_needs] = 1;
                    alredy_enqueued[who_needs] = 0;
                }
                if((who_flush !=4) && need_flush[who_flush] == 1){
                    need_flush[who_flush] = 0;
                    Tsram[who_flush][block_number][0] = '0';
                    Tsram[who_flush][block_number][1] = '0';
                }
                bus_counter = 0;
                bus_busy = 0;
                who_flush = 4;
                who_needs = 4;
                bus_shared[0] = 0;
                bus_shared[1] = 0;
                bus_shared[2] = 0;
                bus_shared[3] = 0;
                
            }
        }
    }
}
