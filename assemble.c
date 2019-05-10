#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINELENGTH 1000

void check(int offnum) {
    if (offnum >= -32768 && Offnum <= 32767) {
    } else {
        printf("Overflow Offset\n");
        exit(1);
    }
}
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
                 char *arg1, char *arg2) {
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
        /* reached end of file */
        return (0);
    }

    /* check for line too long (by looking for a \n) */
    if (strchr(line, '\n') == NULL) {
        /* line too long */
        printf("error: line too long\n");
        exit(1);
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n\r ]", label)) {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r "
                "]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
           opcode, arg0, arg1, arg2);
    return (1);
}

int returnIndex(FILE *inFilePtr2, char *inputlabel) {
    int address;
    int success = 0;
    int counter = 0;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
    rewind(inFilePtr2);
    while (readAndParse(inFilePtr2, label, opcode, arg0, arg1, arg2)) {
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

int Char2Num(char *string) {
    int i;

    sscanf(string, "%d", &i);
    return i;
}

int isNumber(char *string) {
    /* return 1 if string is a number */
    int i;
    return ((sscanf(string, "%d", &i)) == 1);
}


int main(int argc, char *argv[]) {
    char *inFileString, *outFileString;
    FILE *inFilePtr, *inFilePtr2, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
               argv[0]);
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

    // Code to check duplicated labels
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        if (strcmp(label, "")) {
            int same = 0;
            char currentlabel[MAXLINELENGTH];
            strcpy(currentlabel, label);
            while (readAndParse(inFilePtr2, label, opcode, arg0, arg1, arg2)) {
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
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        int opcodeBi;
        int arg0num, arg1num, arg2num, Offnum;
        int mc;

        if (!strcmp(opcode, "add")) {
            opcodeBi = 0;
            arg0num = Char2Num(arg0);
            arg1num = Char2Num(arg1);
            arg2num = Char2Num(arg2);
            Offnum = arg2num;
        } else if (!strcmp(opcode, "nor")) {
            opcodeBi = 1;
            arg0num = Char2Num(arg0);
            arg1num = Char2Num(arg1);
            arg2num = Char2Num(arg2);
            Offnum = arg2num;
        } else if (!strcmp(opcode, "lw")) {
            opcodeBi = 2;
            arg0num = Char2Num(arg0);
            arg1num = Char2Num(arg1);
            if (isNumber(arg2)) {
                arg2num = Char2Num(arg2);
                Offnum = arg2num;
            } else {
                arg2num = returnIndex(inFilePtr2, arg2);
                Offnum = arg2num;
            }
        } else if (!strcmp(opcode, "sw")) {
            opcodeBi = 3;
            arg0num = Char2Num(arg0);
            arg1num = Char2Num(arg1);
            if (isNumber(arg2)) {
                arg2num = Char2Num(arg2);
                Offnum = arg2num;
            } else {
                arg2num = returnIndex(inFilePtr2, arg2);
                Offnum = arg2num;
            }
        } else if (!strcmp(opcode, "beq")) {
            opcodeBi = 4;
            arg0num = Char2Num(arg0);
            arg1num = Char2Num(arg1);
            if (isNumber(arg2)) {
                arg2num = Char2Num(arg2);
                Offnum = arg2num;
            } else {
                arg2num = returnIndex(inFilePtr2, arg2);
                Offnum = arg2num - PC - 1;
            }
        } else if (!strcmp(opcode, "jalr")) {
            opcodeBi = 5;
            arg0num = Char2Num(arg0);
            arg1num = Char2Num(arg1);
            Offnum = 0;
        } else if (!strcmp(opcode, ".fill")) {
            if (isNumber(arg0)) {
                arg0num = Char2Num(arg0);
                mc = arg0num;
                fprintf(outFilePtr, "%d\n", mc);
                PC++;
                continue;
            } else {
                mc = returnIndex(inFilePtr2, arg0);
                fprintf(outFilePtr, "%d\n", mc);
                PC++;
                continue;
            }
        } else if (!strcmp(opcode, "halt")) {
            opcodeBi = 6;
            arg0num = 0;
            arg1num = 0;
            arg2num = 0;
            Offnum = arg2num;
        } else if (!strcmp(opcode, "noop")) {
            opcodeBi = 7;
            arg0num = 0;
            arg1num = 0;
            arg2num = 0;
            Offnum = arg2num;
        } else {
            printf("error: unrecognized opcode\n");
            printf("%s\n", opcode);
            exit(1);
        }
        check(Offnum);
        if (Offnum < 0)
            Offnum += 65536;
        mc = opcodeBi * 4194304 + arg0num * 524288 + arg1num * 65536 + Offnum;
        //    fprintf(outFilePtr, "%d /%d / %d / %d\n", opcodeBi, arg0num, arg1num,
        //        Offnum);

        fprintf(outFilePtr, "%d\n", mc);
        PC++;
    }

    fclose(inFilePtr2);
    fclose(inFilePtr);
    fclose(outFilePtr);
    exit(0);
}
