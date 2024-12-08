#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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
