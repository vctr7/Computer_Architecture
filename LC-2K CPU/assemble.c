#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINELENGTH 1000

void chkOF(int x) {
    if (x >= -32768 && x <= 32767) {
    } else {
        printf("Overflow Offset\n");
        exit(1);
    }
}
int readparse(FILE *inFilePtr, char *label, char *opcode, char *arg0, char *arg1, char *arg2) {
    char line[MAXLINELENGTH];
    char *ptr = line;

    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
        return (0);
    }

    if (strchr(line, '\n') == NULL) {
        printf("error: line too long\n");
        exit(1);
    }

    ptr = line;
    if (sscanf(ptr, "%[^\t\n\r ]", label)) {
        ptr += strlen(label);
    }

    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r "
                "]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
           opcode, arg0, arg1, arg2);
    return (1);
}

int returnindex(FILE *inFilePtr2, char *inputlabel) {
    int address;
    int success = 0;
    int counter = 0;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
    rewind(inFilePtr2);
    while (readparse(inFilePtr2, label, opcode, arg0, arg1, arg2)) {
        if (!strcmp(label, inputlabel)) {
            address = counter;
            success = 1;
        }
        counter++;
    }
    if (success) {
        return address;
    } else {
        printf("Label undefined!\n");
        exit(1);
    }
}

int charnum(char *string) {
    int x;
    sscanf(string, "%d", &x);
    return x;
}

int number(char *string) {
    int x;
    return ((sscanf(string, "%d", &x)) == 1);
}


int main(int argc, char *argv[]) {
    char *inFileString, *outFileString;
    FILE *inFilePtr, *inFilePtr2, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH], arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n", argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    inFilePtr2 = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    while (readparse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        if (strcmp(label, "")) {
            int same = 0;
            char currentlabel[MAXLINELENGTH];
            strcpy(currentlabel, label);
            while (readparse(inFilePtr2, label, opcode, arg0, arg1, arg2)) {
                if (!strcmp(currentlabel, label)) {
                    same++;
                }
            }
            if (same >= 2) {
                printf("Duplicated!\n");
                exit(1);
            }
            rewind(inFilePtr2);
        }
    }

    rewind(inFilePtr);
    rewind(inFilePtr2);
    int PC = 0;
    while (readparse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        int opcodeBi;
        int arg0num, arg1num, arg2num, offnum;
        int mc;

        if (!strcmp(opcode, "add")) {
            opcodeBi = 0;
            arg0num = charnum(arg0);
            arg1num = charnum(arg1);
            arg2num = charnum(arg2);
            offnum = arg2num;
        } 
	else if (!strcmp(opcode, "nor")) {
            opcodeBi = 1;
            arg0num = charnum(arg0);
            arg1num = charnum(arg1);
            arg2num = charnum(arg2);
            offnum = arg2num;
        } 
	else if (!strcmp(opcode, "lw")) {
            opcodeBi = 2;
            arg0num = charnum(arg0);
            arg1num = charnum(arg1);
            if (number(arg2)) {
                arg2num = charnum(arg2);
                offnum = arg2num;
            }
		else {
                	arg2num = returnindex(inFilePtr2, arg2);
               		offnum = arg2num;
            }
        } 
	else if (!strcmp(opcode, "sw")) {
            opcodeBi = 3;
            arg0num = charnum(arg0);
            arg1num = charnum(arg1);
            if (number(arg2)) {
                arg2num = charnum(arg2);
                offnum = arg2num;
            } 
		else {
                	arg2num = returnindex(inFilePtr2, arg2);
               	 	offnum = arg2num;
            }	
        } 
	else if (!strcmp(opcode, "beq")) {
            opcodeBi = 4;
            arg0num = charnum(arg0);
            arg1num = charnum(arg1);
            if (number(arg2)) {
                arg2num = charnum(arg2);
                offnum = arg2num;
            } else {
                arg2num = returnindex(inFilePtr2, arg2);
                offnum = arg2num - PC - 1;
            }
        }
	else if (!strcmp(opcode, "jalr")) {
            opcodeBi = 5;
            arg0num = charnum(arg0);
            arg1num = charnum(arg1);
            offnum = 0;
        }
	else if (!strcmp(opcode, ".fill")) {
            if (number(arg0)) {
                arg0num = charnum(arg0);
                mc = arg0num;
                fprintf(outFilePtr, "%d\n", mc);
                PC++;
                continue;
            } else {
                mc = returnindex(inFilePtr2, arg0);
                fprintf(outFilePtr, "%d\n", mc);
                PC++;
                continue;
            }
        } 
	else if (!strcmp(opcode, "halt")) {
            opcodeBi = 6;
            arg0num = 0;
            arg1num = 0;
            arg2num = 0;
            offnum = arg2num;
        } 
	else if (!strcmp(opcode, "noop")) {
            opcodeBi = 7;
            arg0num = 0;
            arg1num = 0;
            arg2num = 0;
            offnum = arg2num;
        } 
	else {
            printf("error: unrecognized opcode\n");
            printf("%s\n", opcode);
            exit(1);
        }

        chkOF(offnum);

        if (offnum < 0)		offnum += 65536;
        mc = opcodeBi * 4194304 + arg0num * 524288 + arg1num * 65536 + offnum;
        fprintf(outFilePtr, "%d\n", mc);
        PC++;
    }

    fclose(inFilePtr2);
    fclose(inFilePtr);
    fclose(outFilePtr);
    exit(0);
}
