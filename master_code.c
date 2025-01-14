#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>


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
void alu(int core_num);
void mem(int core_num);
void wb(int core_num);
void MESI_bus();
int data_in_dsram(int core_num, int address);
int is_branch(char *command);
void int_to_hex(int x, char * hex);
int hex_to_int(char * hex);
void bin_to_hex(char *bin, char *hex);
void hex_to_bin(char *bin,char*hex);
int arithmetic_shift_right(int num, int shift);
void slice_inst(char * input, int output[]);
void read_file_into_array_memin(const char *filename, char ***array, int *line_count);
int read_file(const char *filename, char lines[MAX_LINES][LINE_LENGTH]);

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
char** memout;
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
int reg_is_available[NUMBER_CORES][NUMBER_REGS];
int flag_decode_stall[NUMBER_CORES];
int flag_mem_stall[NUMBER_CORES];
FILE* core_trace_files[NUMBER_CORES];
FILE* bus_trace_file;



// char* filename,   char ***array,   int*;
void main(int argc, char *argv[]){
    FILE* memout_file;
    FILE* regout_files[NUMBER_CORES];
    FILE* dsram_files[NUMBER_CORES];
    FILE* tsram_files[NUMBER_CORES];
    FILE* stats_files[NUMBER_CORES];

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

    int memin_lines;
    int line_count;
    line_count = read_file(imem0_txt, Imem0);
    if (line_count < 0) {
            printf("Failed to read file: %s\n", imem0_txt);
        }

    line_count = read_file(imem1_txt, Imem1);
    if (line_count < 0) {
            printf("Failed to read file: %s\n", imem1_txt);
        }

    line_count = read_file(imem2_txt, Imem2);
    if (line_count < 0) {
            printf("Failed to read file: %s\n", imem2_txt);
        }

    line_count = read_file(imem3_txt, Imem3);
    if (line_count < 0) {
            printf("Failed to read file: %s\n", imem3_txt);
        }

    read_file_into_array_memin(memin_txt, &memout, &memin_lines);
    for (i = memin_lines; i < MAX_LINE_MEMIN; i++) {
        memout[i] = strdup("00000000");
        if (!memout[i]) {
            perror("Memory allocation failed while adding padding to memout");
            return EXIT_FAILURE;
        }
    }

    memout_file = fopen(memout_txt, "w");
    regout_files[0] = fopen(regout0_txt, "w");
    regout_files[1] = fopen(regout1_txt, "w");
    regout_files[2] = fopen(regout2_txt, "w");
    regout_files[3] = fopen(regout3_txt, "w");
    core_trace_files[0] = fopen(core0trace_txt, "w");
    core_trace_files[1] = fopen(core1trace_txt, "w");
    core_trace_files[2] = fopen(core2trace_txt, "w");
    core_trace_files[3] = fopen(core3trace_txt, "w");
    bus_trace_file = fopen(bustrace_txt, "w");
    dsram_files[0] = fopen(dsram0_txt, "w");
    dsram_files[1] = fopen(dsram1_txt, "w");
    dsram_files[2] = fopen(dsram2_txt, "w");
    dsram_files[3] = fopen(dsram3_txt, "w");
    tsram_files[0] = fopen(tsram0_txt, "w");
    tsram_files[1] = fopen(tsram1_txt, "w");
    tsram_files[2] = fopen(tsram2_txt, "w");
    tsram_files[3] = fopen(tsram3_txt, "w");
    stats_files[0] = fopen(stats0_txt, "w");
    stats_files[1] = fopen(stats1_txt, "w");
    stats_files[2] = fopen(stats2_txt, "w");
    stats_files[3] = fopen(stats3_txt, "w");


    for(i=0; i<NUMBER_CORES; i++){
        for(c=0; c<8; c++){
            memset(&pipe_regs[i][c], 0, sizeof(pipe_regs[i][c]));
            pipe_regs[i][c].pc_pipe = -1;
        }
        for(k=0; k<NUMBER_REGS; k++){
            regs[i][k] = 0;
            reg_is_available[i][k] = 1;
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
        flag_decode_stall[i] = 0;
        flag_mem_stall[i] = 0;
    }
    bus_counter = 0;
    initializeQueue(&core_bus_requests);
    bus_busy = 0;
    who_flush = 4;
    who_needs = 4; 



    // here we start after init all the global variables and the file names
    // start with if busy then MESI()


    while(1){
        for(i=0; i<4; i++){
            if(pc[i] == -1 && pipe_regs[i][6].pc_pipe == -2){
                // here we can write the total cycles in states file
                cycles[i] = -1;
                continue;
            }

            fprintf(core_trace_files[i],"%d ",cycles[i]);
            
            fetch(i);
            decode(i);
            alu(i);
            mem(i);
            wb(i);
            MESI_bus();
            pipe_regs[i][6] = pipe_regs[i][7]; // write back continues
            if(flag_decode_stall[i] || flag_mem_stall[i]){ // if stall
                pc[i] --; //fetch the same pc
                if(flag_decode_stall[i] && (!flag_mem_stall[i])){ // if just decode stall
                    pipe_regs[i][4] = pipe_regs[i][5]; // mem continues
                    pipe_regs[i][2] = pipe_regs[i][3]; // alu continues
                    continue;
                }
            }
            else{
                pipe_regs[i][4] = pipe_regs[i][5];
                pipe_regs[i][2] = pipe_regs[i][3];
                pipe_regs[i][0] = pipe_regs[i][1];
            }
            if(cycles[i] != -1){
                cycles[i]++;
            }

            for(c=2; c<=14; c++){
                fprintf(core_trace_files[i], "%08X ",regs[i][c]);
            }
            fprintf(core_trace_files[i], "%08X\n",regs[i][15]);

        }
        if(cycles[0] == -1 && cycles[1] == -1 && cycles[2] == -1 && cycles[3] == -1){
            break;
        }

    }

    /////////////////////////////////////////////print memout + regout0-3 + stats+ dsram + tsram
}


void read_file_into_array_memin(const char *filename, char ***array, int *line_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the array of strings
    *array = malloc(MAX_LINE_MEMIN * sizeof(char *));
    if (!*array) {
        perror("Memory allocation failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char buffer[LINE_LENGTH]; // Buffer to store each line
    *line_count = 0;

    // Read each line from the file
    while (fgets(buffer, sizeof(buffer), file)) {
        // Remove the newline character if present
        buffer[strcspn(buffer, "\n")] = '\0';

        // Allocate memory for the line and store it
        (*array)[*line_count] = strdup(buffer);
        if (!(*array)[*line_count]) {
            perror("Memory allocation failed");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        (*line_count)++;
    }

    fclose(file);
}


int read_file(const char *filename, char lines[MAX_LINES][LINE_LENGTH]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1; // Indicate error
    }

int line_count = 0;
    while (fgets(lines[line_count], LINE_LENGTH, file) && line_count < MAX_LINES) {
        // Remove newline character if present
        lines[line_count][strcspn(lines[line_count], "\n")] = '\0';
        line_count++;
    }

    fclose(file);
    return line_count; // Return the number of lines read
}

void mem(int core_num){  
    if(pc[core_num] == -1 && pipe_regs[core_num][4].pc_pipe == -2){ // reached halt
        fprintf(core_trace_files[core_num], "--- ");
        pipe_regs[core_num][7].pc_pipe = -2;
        return;
    }
    if(pipe_regs[core_num][4].pc_pipe == -1){
        fprintf(core_trace_files[core_num], "--- ");
        pipe_regs[core_num][7].pc_pipe = -1;
        return;
    }                              
    int data = 0;
    int op = pipe_regs[core_num][4].op;
    int address = pipe_regs[core_num][4].ALU_pipe;                    
    int wanted_tag = address / pow(2,8); // assuming 20 bit address   // tag is the first 8 bits address 
    int index = address % 256;
    int block = (index / 4);
    char *tr = Tsram[core_num][block];            //[TAG][MESI] block as in the assignment 
    char *tag = tr+2;                                // to ditch first two 
    char tag_hex[4]; // may need to make it 3
    bin_to_hex(tag, tag_hex);
    int tag_num =  hex_to_int(tag_hex);                 // same tag but in int
    int mesi_state = -1;
    if(*tr == '0'){ // calculating messi state
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
                    flag_mem_stall[core_num] = 1;
                }
            }
            else{ // tag is not matched
                if(mesi_state == 0 || mesi_state == 2){//invalid or exclusive
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, BusRd);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                    flag_mem_stall[core_num] = 1;
                }
                else{ // modified or shared
                    need_flush[core_num] = 1;
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, Flush);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                    flag_mem_stall[core_num] = 1;
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
                    flag_mem_stall[core_num] = 1;
                }
            }
            else{//tag not matched
                if(mesi_state == 0 || mesi_state == 2){//invalid or exclusive
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, BusRdX);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                    flag_mem_stall[core_num] = 1;
                }
                else{ // modified or shared
                    need_flush[core_num] = 1;
                    enqueue(&core_bus_requests, core_num);
                    enqueue(&bus_origid, core_num);
                    enqueue(&bus_cmd, Flush);
                    enqueue(&bus_address, address);
                    core_ready[core_num] = 0; // my data is not ready
                    flag_mem_stall[core_num] = 1;
                }
            }
        }
    }
    if(core_ready[core_num] == 1){
        flag_mem_stall[core_num] = 0;
        if(op == 16){ // lw
            data = hex_to_int(Dsram[core_num][index]);
        }
        if(op == 17){ // sw
            int_to_hex(pipe_regs[core_num][4].rd, Dsram[core_num][index]);
        }

        // sending all the data to the next pipe register
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

// function to check if the data in the dsram (return the core number)
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
            bin_to_hex(tag, tag_hex);
            int tag_num =  hex_to_int(tag_hex);
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
        if(bus_counter>0 && bus_counter<5){ // the data is shared ,then we start giving the words from the first cycle
            if(bus_sharing == 1){
                strcpy(Dsram[who_needs][bus_address + bus_counter - 1], Dsram[who_flush][bus_address + bus_counter - 1]);
            }
        }
        if(bus_counter >= 16){
            if((!bus_sharing == 1) && who_needs != 4){ // the data was not shared (giving the core)
                strcpy(Dsram[who_needs][bus_address + bus_counter - 16], Dsram[who_flush][bus_address + bus_counter - 16]);
            }
            if(who_flush != 4){ // main memory not flushing then we give it data
                strcpy(memout[bus_address + bus_counter - 16], Dsram[who_flush][bus_address  + bus_counter - 16]);
            }
            if(bus_counter == 19){ //we gave all the words neede and returning to the default settings
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

int is_branch(char *command){
    if(command[0] == '0' && (command[1]=='9' || command[1]=='a' || command[1]=='b' || command[1]=='c' || command[1]=='d' || command[1]=='e' || command[1]=='f'))
        return 1;
    else
        return 0;
}
void int_to_hex(int x, char * hex){
    snprintf(hex, 9, "%08X", x);
}
int hex_to_int(char * hex){
    return ((int)strtol(hex, NULL, 16));
}
void bin_to_hex(char *bin, char *hex) {
    int binLen = strlen(bin);

    // Step 1: Convert binary string to an integer
    int number = 0;
    for (int i = 0; i < binLen; i++) {
        if (bin[i] == '1') {
            number = (number << 1) | 1; // Shift left and set the least significant bit
        } else if (bin[i] == '0') {
            number = number << 1; // Shift left
        } else {
            printf("Invalid binary input.\n");
            return;
        }
    }

    // Step 2: Convert integer to hexadecimal, ensuring 8-character length
    snprintf(hex, 9, "%08X", number);
}
void hex_to_bin(char *bin,char*hex)    { // Lookup table to convert hex digits to binary strings
    char *bin_lookup[] = {
        "0000", "0001", "0010", "0011", 
        "0100", "0101", "0110", "0111", 
        "1000", "1001", "1010", "1011", 
        "1100", "1101", "1110", "1111"
    };

    int binIndex = 0;

    // Iterate over each hex digit and map to binary
    for (int i = 0; i < strlen(hex); i++) {
        char hexChar = hex[i];
        int value;

        // Determine the value of the hex character
        if (hexChar >= '0' && hexChar <= '9') {
            value = hexChar - '0';
        } else if (hexChar >= 'A' && hexChar <= 'F') {
            value = hexChar - 'A' + 10;
        } else if (hexChar >= 'a' && hexChar <= 'f') {
            value = hexChar - 'a' + 10;
        } else {
            printf("Invalid hexadecimal character: %c\n", hexChar);
            bin[0] = '\0'; // Set the binary output to an empty string
            return;
        }

        // Append the binary representation of the hex value
        strcpy(&bin[binIndex], bin_lookup[value]);
        binIndex += 4; // Each hex digit corresponds to 4 binary digits
    }

    // Null-terminate the binary string
    bin[binIndex] = '\0';
}

void fetch (int i){ // Takes pc[i] and the instruction of the pc and put pc++ and the instruction in pipe_regs[i][1] .
    int h = pc[i];
    if(h == -1){
        fprintf(core_trace_files[i], "--- ");
        pipe_regs[i][1].pc_pipe = -2; // -2 = halt
        return;
    }
    fprintf(core_trace_files[i], "%03d ", h); //printing the pc in fetch
    h=h+1;
    pipe_regs[i][1].pc_pipe=h;
    switch (i) {
        case 0:
            for (int k=0;k<LINE_LENGTH;k++){
                pipe_regs[i][1].inst[k]=Imem0[pc[i]][k];
                if(pipe_regs[i][1].inst[0] == '1' && pipe_regs[i][1].inst[1] == '4'){
                    pipe_regs[i][1].pc_pipe = -1;
                    pc[i] = -1;
                }

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
    if(pc[i] == -1 && pipe_regs[i][2].pc_pipe == -2){ // reached halt
        fprintf(core_trace_files[i], "--- ");
        pipe_regs[i][5].pc_pipe = -2;
        return;
    }
    if(pipe_regs[i][2].pc_pipe == -1){
        fprintf(core_trace_files[i], "--- ");
        pipe_regs[i][5].pc_pipe = -1;
        return;
    }
    int optag = pipe_regs[i][2].op;
    int rstag = pipe_regs[i][2].rs;
    int rttag = pipe_regs[i][2].rt;
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
	case 16:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] + regs[i][rttag];
            break;
	case 17:
            pipe_regs[i][5].ALU_pipe=regs[i][rstag] + regs[i][rttag];
            break;
    }
}
void wb (int i){ //Takes the opcode , destination , output of alu and data from memory (from pipe_regs[i][6]) and according to the opcode he put the correct result in the destination register .
    if(pc[i] == -1 && pipe_regs[i][6].pc_pipe == -2){ // reached halt
        cycles[i] = -1;
        return;
    }
    if(pipe_regs[i][6].pc_pipe == -1){
        fprintf(core_trace_files[i],"--- ");
    }
    int optag = pipe_regs[i][6].op;
    int disttag = pipe_regs[i][6].dist;//the index of the dist register(rd)
    int ALU_pipetag = pipe_regs[i][6].ALU_pipe;//outp of the alu
    int datatag = pipe_regs[i][6].data;// data from memory
    if (optag==0 || optag==1 || optag==2 || optag==3 || optag==4 || optag==5 || optag==6 || optag==7 || optag==8) {
        regs[i][disttag]=ALU_pipetag;
        reg_is_available[i][disttag] = 1;

    }
    if(optag == 15){
        regs[i][15] = pipe_regs[i][6].pc_pipe;
        reg_is_available[i][15] = 1;
    }
    else if (optag==16){
        regs[i][disttag]=datatag;
        reg_is_available[i][disttag] = 1;
    }

}

void slice_inst(char * input, int output[]) { // add int sliced_inst[5];

    // Convert hexadecimal string input to integer
    uint32_t instruction = (uint32_t)strtol(input, NULL, 16);

    output[0] = (instruction >> 24) & 0xFF;       // Bits 31:24 (op)
    output[1] = (instruction >> 20) & 0x0F;       // Bits 23:20 (rd)
    output[2] = (instruction >> 16) & 0x0F;       // Bits 19:16 (rs)
    output[3] = (instruction >> 12) & 0x0F;       // Bits 15:12 (rt)
    output[4] = instruction & 0x0FFF;             // Bits 11:0 (imm)
}


void decode(int i){
    if(pc[i] == -1 && pipe_regs[i][0].pc_pipe == -2){ // reached halt
        pipe_regs[i][3].pc_pipe = -2;
        fprintf(core_trace_files[i], "--- ");
        return;
    }
    if(pipe_regs[i][0].pc_pipe == -1){
        fprintf(core_trace_files[i], "--- ");
        return;
    }

    int sliced_inst[5] = {0};
    int reg_d = 0;
    int reg_s = 0;
    int reg_t = 0;
    int my_imm = 0;
    //should each of these be done on each attribute {op,rd,rs,rt...?}
    slice_inst(pipe_regs[i][0].inst, sliced_inst);

    if(sliced_inst[0] != 15 && sliced_inst[0] != 20){ // rd is not a source reg
        if(!(reg_is_available[i][sliced_inst[2]] && reg_is_available[i][sliced_inst[3]])){
            if(reg_is_available[i][sliced_inst[0]] == 17){ // if op is sw we should check if rd is available
                if(!reg_is_available[i][sliced_inst[1]]){
                    decode_stall[i] ++;
                    flag_decode_stall[i] = 1;
                    pipe_regs[i][3].pc_pipe = -1;
                    return;
                }
            }
            decode_stall[i] ++;
            flag_decode_stall[i] = 1;
            pipe_regs[i][3].pc_pipe = -1;
            return;
        }
    }
    else if(sliced_inst[0] == 15){
        if(!(reg_is_available[i][sliced_inst[1]] )){
            decode_stall[i] ++;
            flag_decode_stall[i] = 1;
            pipe_regs[i][3].pc_pipe = -1;
            return;
        }
    }

    flag_decode_stall[i] = 0; // all source regs are ready

    //pc_pipe
    pipe_regs[i][3].pc_pipe = pipe_regs[i][0].pc_pipe;
    //inst
    for (int k=0;k<LINE_LENGTH;k++){
        pipe_regs[i][3].inst[k]=pipe_regs[i][0].inst[k];
    }
    
    pipe_regs[i][3].op = sliced_inst[0];
    
    my_imm = regs[i][sliced_inst[4]];

    //rd
    if(sliced_inst[1] == 1){
        pipe_regs[i][3].rd = my_imm;
    }
    else{
        reg_d = regs[i][sliced_inst[1]];
        pipe_regs[i][3].rd = reg_d;// might need to be converted to decimal from hex
    }

    //rs
    if(sliced_inst[2] == 1){
        pipe_regs[i][3].rs = my_imm;
    }
    else{
        reg_s = regs[i][sliced_inst[2]];
        pipe_regs[i][3].rs = reg_s;// might need to be converted to decimal from hex
    }

    //rt
    if(sliced_inst[3] == 1){
        pipe_regs[i][3].rt = my_imm;
    }
    else{
        reg_t = regs[i][sliced_inst[3]];
        pipe_regs[i][3].rt = reg_t;// might need to be converted to decimal from hex
    }

    //imm
    pipe_regs[i][3].imm = my_imm;// might need to be converted to decimal from hex
    //dist 
    pipe_regs[i][3].dist = sliced_inst[1];
    

    //branching 

    if(is_branch(pipe_regs[i][0].inst)){
        switch(sliced_inst[0]){
            case 9:
                if(sliced_inst[2] == 1 && sliced_inst[3]){ // rs and rt is imm
                        pc[i] =  (reg_d & 0x3FF);
                        return;
                }
                if(sliced_inst[2] == 1){ // just rs is imm
                    if(reg_t == my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                if(sliced_inst[3] == 1){ // just rt is imm
                    if(reg_s == my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                else{ // no imm
                    if(reg_s == reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
            
            case 10:
                if(sliced_inst[2] == 1 && sliced_inst[3]){ // rs and rt is imm
                        return;
                }
                if(sliced_inst[2] == 1){ // just rs is imm
                    if(reg_t != my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                if(sliced_inst[3] == 1){ // just rt is imm
                    if(reg_s != my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                else{ // no imm
                    if(reg_s != reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }

            case 11:
                if(sliced_inst[2] == 1 && sliced_inst[3]){ // rs and rt is imm
                        return;
                }
                if(sliced_inst[2] == 1){ // just rs is imm
                    if(my_imm < reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                if(sliced_inst[3] == 1){ // just rt is imm
                    if(reg_s < my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                else{ // no imm
                    if(reg_s < reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }

            case 12:
                if(sliced_inst[2] == 1 && sliced_inst[3]){ // rs and rt is imm
                        return;
                }
                if(sliced_inst[2] == 1){ // just rs is imm
                    if(my_imm > reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                if(sliced_inst[3] == 1){ // just rt is imm
                    if(reg_s > my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                else{ // no imm
                    if(reg_s > reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }

                case 13:
                    if(sliced_inst[2] == 1 && sliced_inst[3]){ // rs and rt is imm
                        pc[i] =  (reg_d & 0x3FF);
                        return;
                }
                if(sliced_inst[2] == 1){ // just rs is imm
                    if(my_imm <= reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                if(sliced_inst[3] == 1){ // just rt is imm
                    if(reg_s <= my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                else{ // no imm
                    if(reg_s <= reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }

            case 14:
                if(sliced_inst[2] == 1 && sliced_inst[3]){ // rs and rt is imm
                        pc[i] =  (reg_d & 0x3FF);
                        return;
                }
                if(sliced_inst[2] == 1){ // just rs is imm
                    if(my_imm >= reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                if(sliced_inst[3] == 1){ // just rt is imm
                    if(reg_s >= my_imm){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }
                else{ // no imm
                    if(reg_s >= reg_t){
                        pc[i] =  (reg_d & 0x3FF);
                    }
                    return;
                }

            case 15:
                reg_is_available[i][15] = 0;
                pc[i] =  (reg_d & 0x3FF);
                return;

        }
    }
    if((sliced_inst[0] >=0 && sliced_inst[0] <=8) || sliced_inst[0] ==16){ // making the distenation reg not available for decoding next instructions.
        reg_is_available[i][sliced_inst[1]] = 0;
    }

}    
        


