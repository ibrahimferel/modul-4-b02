#include "std_lib.h"
#include "kernel.h"

// Insert global variables and function prototypes here
void command(char* buf);
void commandEcho(char* arg, char* pipeArg);
void commandGrep(char* input, char* pattern, char* outputBuffer);
void commandWc(char* input);
void intToStr(int val, char* buf);

int main() {
    char buf[128];

    clearScreen();
    printString("LilHabOS - [[PUT YOUR TEAM CODE HERE]]\n");

    while (true) {
        printString("$> ");
        readString(buf);
        printString("\n");

        if (strlen(buf) > 0) {
            // Insert your functions here, you may not need to modify the rest of the main() function
            command(buf);
        }
    }
}

// Insert function here
void printString(char* str){
    int i;
    for(i = 0; str[i] != '\0'; i++){
        if(str[i] == '\n'){
            interrupt(0x10, 0x0E0D, 0, 0, 0);
            interrupt(0x10, 0x0E0A, 0, 0, 0);
        } else{
            interrupt(0x10, 0x0E00 | str[i], 0, 0, 0);
        }
    }
}

void readString(char* buf){
    int i = 0, karakter = 0;
    while(1){
        karakter = interrupt(0x16, 0x0000, 0, 0, 0) & 0xFF;
        if(karakter == 0x0D){
            buf[i] = '\0';
            interrupt(0x10, 0x0E0A, 0, 0, 0);
            interrupt(0x10, 0x0E0D, 0, 0, 0);
            break;
        } else if(karakter == 0x08 && i > 0){
            i--;
            interrupt(0x10, 0x0E08, 0, 0, 0);
            interrupt(0x10, 0x0E20, 0, 0, 0);
            interrupt(0x10, 0x0E08, 0, 0, 0);
        } else if(karakter >= 32 && karakter <= 126){
            buf[i++] = karakter;
            interrupt(0x10, 0x0E00 | karakter, 0, 0, 0);
        }
    }
}

void clearScreen(){
    interrupt(0x10, 0x0600, 0x0700, 0x0000, 0x184F);
    interrupt(0x10, 0x0200, 0, 0, 0);
}

void intToStr(int val, char* buf){
    int i = 0, j;
    char temp[10];
    if(val == 0){
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while(val > 0){
        temp[i++] = '0' + mod(val,10);
        val = div(val,10);
    }
    for(j = 0; j < i; j++) buf[j] = temp[i-j-1];
    buf[i] = '\0';
}

void command(char* buf){
    char part[3][128];
    char arg[128], pipeBuffer1[128], pipeBuffer2[128];
    char cmd[32], cmdArg[128];
    int countPart = 0, i = 0, j = 0, k;

    clear((byte*)arg, 128);
    clear((byte*)pipeBuffer1, 128);
    clear((byte*)pipeBuffer2, 128);

    for(k = 0; k < 3; k++){
        clear((byte*)part[k], 128);
    }

    while(buf[i] != '\0' && countPart < 3){
        if(buf[i] == '|'){
            part[countPart][j] = '\0';
            countPart++;
            j = 0;
            i++;
	        while(buf[i] == ' ') i++;
            continue;
        }
        part[countPart][j++] = buf[i++];
    }
    part[countPart][j] = '\0';
    countPart++;

    j = strlen(part[0]) - 1;
    while(j >= 0 && part[0][j] == ' '){
        part[0][j] = '\0';
        j--;
    }

    i = 0;
    j = 0;
    while(part[0][i] != ' ' && part[0][i] != '\0'){
        cmd[j++] = part[0][i++];
    }
    cmd[j] = '\0';

    if(strcmp(cmd, "echo")){
        while(part[0][i] == ' ') i++;

        j = 0;
        while(part[0][i] != '\0'){
            arg[j++] = part[0][i++];
        }
        arg[j] = '\0';
    } else{
        printString("Command not recognized\n");
        return;
    }

    if(countPart == 1){
        commandEcho(arg, NULL);
    } else if(countPart == 2){
        commandEcho(arg, pipeBuffer1);

        i = 0; j = 0;
        while(part[1][i] == ' ') i++;
        while(part[1][i] != '\0'){
            part[1][j++] = part[1][i++];
        }
        part[1][j] = '\0';

        i = 0; j = 0;
        clear((byte*)cmd, 32);
        clear((byte*)cmdArg, 128);

        while(part[1][i] != ' ' && part[1][i] != '\0'){
            cmd[j++] = part[1][i++];
        }
        cmd[j] = '\0';

        if(strcmp(cmd, "grep")){
            while(part[1][i] == ' ') i++;
            j = 0;
            while(part[1][i] != '\0'){
                cmdArg[j++] = part[1][i++];
            }
            cmdArg[j] = '\0';
            commandGrep(pipeBuffer1, cmdArg, NULL);
        } else if(strcmp(cmd, "wc")){
            commandWc(pipeBuffer1);
        } else{
            printString("Command not recognized\n");
        }
    } else if(countPart == 3){
        commandEcho(arg, pipeBuffer1);

        for(k = 1; k < 3; k++){
            i = 0; j = 0;
            while(part[k][i] == ' ') i++;
            while(part[k][i] != '\0') {
                part[k][j++] = part[k][i++];
            }
            part[k][j] = '\0';
        }

        i = 0; j = 0;
        clear((byte*)cmd, 32);
        clear((byte*)cmdArg, 128);

        while(part[1][i] != ' ' && part[1][i] != '\0'){
            cmd[j++] = part[1][i++];
        }
        cmd[j] = '\0';

        if(strcmp(cmd, "grep")){
            while(part[1][i] == ' ') i++;
            j = 0;
            while(part[1][i] != '\0'){
                cmdArg[j++] = part[1][i++];
            }
            cmdArg[j] = '\0';
            commandGrep(pipeBuffer1, cmdArg, pipeBuffer2);

            if(strcmp(part[2], "wc")){
                commandWc(pipeBuffer2);
            } else{
                printString("Command not recognized\n");
            }
        } else{
            printString("Command not recognized\n");
        }
    } else{
        printString("Command not recognized\n");
    }
}

void commandEcho(char* arg, char* pipeArg){
    if(pipeArg == NULL){
        printString(arg);
        printString("\n");
    } else{
        strcpy(arg, pipeArg);
    }
}

void commandGrep(char* input, char* pattern, char* outputBuffer){
    int lenText = strlen(input), lenPat = strlen(pattern);
    int i, j, match, found = 0;

    while(lenPat > 0 && (pattern[lenPat - 1] == ' ' || pattern[lenPat - 1] == '\n')){
        pattern[lenPat - 1] = '\0';
        lenPat--;
    }

    for(i = 0; i <= lenText - lenPat; i++){
        match = 1;
        for(j = 0; j < lenPat; j++){
            if(input[i+j] != pattern[j]){
                match = 0;
                break;
            }
        }
        if(match){
            found = 1;
            break;
        }
    }
    if(found){
        if(outputBuffer != NULL){
            strcpy(pattern, outputBuffer);
        } else{
            printString(pattern);
            printString("\n");
        }
    } else{
        if(outputBuffer != NULL){
            clear(outputBuffer,128);
        } else{
            printString("NULL\n");
        }
    }
}

void commandWc(char* input){
    int charCount = 0, wordCount = 0, lineCount = 1, inWord = 0, i = 0;
    char buf[16];

    if(strlen(input) == 0){
        printString("Baris: 0, Kata: 0, Karakter: 0\n");
        return;
    }

    while(input[i] != '\0'){
        charCount++;
        if(input[i] == ' ' || input[i] == '\t'){
            if(inWord){
                wordCount++;
                inWord = 0;
            }
        } else if(input[i] == '\n'){
            lineCount++;
            if(inWord){
                wordCount++;
                inWord = 0;
            }
        } else{
            inWord = 1;
        }
        i++;
    }
    if(inWord) wordCount++;

    printString("Baris: ");
    intToStr(lineCount, buf);
    printString(buf);

    printString(", Kata: ");
    intToStr(wordCount, buf);
    printString(buf);

    printString(", Karakter: ");
    intToStr(charCount, buf);
    printString(buf);
    printString("\n");
}
