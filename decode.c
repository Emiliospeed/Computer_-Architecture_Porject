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



//read_memin

#define MAX_LINES_MEMIN 1048576  // Maximum number of lines in memin.txt
#define LINE_LENGTH 9            // Length of each line (8 characters + 1 null terminator)

void read_file_into_array_memin(const char *filename, char ***array, int *line_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the array of strings
    *array = malloc(MAX_LINES_MEMIN * sizeof(char *));
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

int main() {
    char **memin;   // Array to hold lines from memin.txt
    char **memout;  // Array to hold extended lines
    int memin_lines; // Number of lines in memin.txt

    // Read memin.txt into the memin array
    read_file_into_array_memin("memin.txt", &memin, &memin_lines);

    // Allocate memory for memout (2^20 lines)
    memout = malloc(MAX_LINES_MEMIN * sizeof(char *));
    if (!memout) {
        perror("Memory allocation failed for memout");
        return EXIT_FAILURE;
    }

    // Copy lines from memin to memout
    for (int i = 0; i < memin_lines; i++) {
        memout[i] = strdup(memin[i]);
        if (!memout[i]) {
            perror("Memory allocation failed while copying to memout");
            return EXIT_FAILURE;
        }
    }

    // Add "00000000" padding to memout to reach 2^20 lines
    for (int i = memin_lines; i < MAX_LINES_MEMIN; i++) {
        memout[i] = strdup("00000000");
        if (!memout[i]) {
            perror("Memory allocation failed while adding padding to memout");
            return EXIT_FAILURE;
        }
    }



    return 0;
}

//read_imem
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_FILES 5      // Number of files
#define MAX_LINES 1024    // Maximum lines per file
#define MAX_LINE_LENGTH 8 // Maximum characters per line

// Function to read a file and store its content into a 2D array
int read_file(const char *filename, char lines[MAX_LINES][MAX_LINE_LENGTH]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1; // Indicate error
    }

int line_count = 0;
    while (fgets(lines[line_count], MAX_LINE_LENGTH, file) && line_count < MAX_LINES) {
        // Remove newline character if present
        lines[line_count][strcspn(lines[line_count], "\n")] = '\0';
        line_count++;
    }

    fclose(file);
    return line_count; // Return the number of lines read
}
int main() {
    const char *filenames[MAX_FILES] = {"imem0.txt", "imem1.txt", "imem2.txt", "imem3.txt", "memin.txt"};
    char file_contents[MAX_FILES][MAX_LINES][MAX_LINE_LENGTH];

    for (int i = 0; i < MAX_FILES; i++) {
        int line_count = read_file(filenames[i], file_contents[i]);
        if (line_count < 0) {
            printf("Failed to read file: %s\n", filenames[i]);
            continue;
        }

        printf("Contents of %s:\n", filenames[i]);
        for (int j = 0; j < line_count; j++) {
            printf("Line %d: %s\n", j + 1, file_contents[i][j]);
        }
        printf("\n");
    }

    // Example: Accessing a specific line from a specific file
    int file_index = 0; // First file
    int line_number = 2; // Line number 2 (index starts from 1)
    if (line_number - 1 < MAX_LINES) {
        printf("File %s, Line %d: %s\n", filenames[file_index], line_number, file_contents[file_index][line_number - 1]);
    }

    return 0;
}

