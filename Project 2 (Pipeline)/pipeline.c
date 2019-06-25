#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */

/*
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5
#define HALT 6
#define NOOP 7
*/
#define MAXLINELENGTH 1000
#define NOOPINSTRUCTION 0x1c00000

typedef enum OPCODE {
    ADD, NOR, LW, SW, BEQ, JALR, HALT, NOOP
} OPCODE;

typedef struct IFIDStruct {
	int instr;
	int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
} IDEXType;

typedef struct EXMEMStruct {
	int instr;
	int branchTarget;
	int aluResult;
	int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
	int instr;
	int writeData;
} MEMWBType;

typedef struct WBENDStruct {
	int instr;
	int writeData;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* number of cycles run so far */
} stateType;



void run(stateType *state, stateType *newState);
void initState(stateType *state);
bool isHazard(stateType *state);
void forwardRegOnHazard(stateType *state,int *inp1,int *inp2);
int convertNum(int num);
void printBinary(int ml);
void errExit(char *str);

void printState(stateType *statePtr);
int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);
void printInstruction(int instr);

int main(int argc, char *argv[])
{
	stateType *state, *newState;
	char line[MAXLINELENGTH];
	FILE *filePtr;
	state = malloc(sizeof(stateType));
	newState = malloc(sizeof(stateType));
	if (argc != 2) {
		printf("error: usage: %s <machine-code file>\n", argv[0]);
		exit(1);
	}

	filePtr = fopen(argv[1], "r");
	if (filePtr == NULL) {
		printf("error: can't open file %s", argv[1]);
		perror("fopen");
		exit(1);
	}
	/* Init machine */
	initState(state);

	/* read in the entire machine-code file into memory */
	for (state->numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
			state->numMemory++) {

		if (sscanf(line, "%d", state->instrMem+state->numMemory) != 1) {
			printf("error in reading address %d\n", state->numMemory);
			exit(1);
		}
		state->dataMem[state->numMemory] = state->instrMem[state->numMemory];
		printf("memory[%d]=%d\n", state->numMemory, state->instrMem[state->numMemory]);
	}

	printf("%d memory words\n", state->numMemory);
	printf("\tinstruction memory:\n");
	for (int i = 0; i < state->numMemory; i++) {
		printf("\t\tinstrMem[ %d ] ", i);
		printInstruction(state->instrMem[i]);
	}


	/* Init machine finished*/

	run(state, newState);
}

void run(stateType *state, stateType *newState)
{
	while (1) {
		printState(state);

		/* check for halt */
		if (opcode(state->MEMWB.instr) == HALT) {
			printf("machine halted\n");
			printf("total of %d cycles executed\n", state->cycles);
			exit(0);
		}

		// *newState = *state;
		memcpy(newState, state, sizeof(stateType));
		newState->cycles++;

		/* --------------------- IF  stage --------------------- */
		newState->IFID.instr = state->instrMem[state->pc];
		newState->IFID.pcPlus1 = newState->pc = state->pc+1;

		/* --------------------- ID  stage --------------------- */
		if (isHazard(state)) {// 해저드 생길 시 현재 상태를 Stall하고 다음 싸이클에 다시 Fetch
			newState->IFID.instr = state->IFID.instr;
			newState->IFID.pcPlus1 = newState->pc = state->pc;
			newState->IDEX.instr = NOOPINSTRUCTION;
			newState->IDEX.readRegA = 0;
			newState->IDEX.readRegB = 0;
            newState->IDEX.pcPlus1 = 0;
			newState->IDEX.offset = 0;
		}
		else { //기존 코드
			newState->IDEX.instr = state->IFID.instr;
			newState->IDEX.pcPlus1 = state->IFID.pcPlus1;
			newState->IDEX.readRegA = state->reg[field0(state->IFID.instr)];
			newState->IDEX.readRegB = state->reg[field1(state->IFID.instr)];
			// sign-extension is happening within ID stage
			newState->IDEX.offset = convertNum(field2(state->IFID.instr));
		}

		/* --------------------- EX  stage --------------------- */
		newState->EXMEM.instr = state->IDEX.instr;
		newState->EXMEM.branchTarget = state->IDEX.pcPlus1 + state->IDEX.offset;
		
        int muxRegA = state->IDEX.readRegA , muxRegB = state->IDEX.readRegB;
        forwardRegOnHazard(state, &muxRegA, &muxRegB);//해저드
        
        
		if (opcode(state->IDEX.instr) == ADD) {
			newState->EXMEM.aluResult = muxRegA + muxRegB;
		} else if (opcode(state->IDEX.instr) == NOR) {
			newState->EXMEM.aluResult = ~(muxRegA | muxRegB);
		} else if (opcode(state->IDEX.instr) == LW) {
			newState->EXMEM.aluResult = muxRegA + state->IDEX.offset;
		} else if (opcode(state->IDEX.instr) == SW) {
			newState->EXMEM.aluResult = muxRegA + state->IDEX.offset;
		} else if (opcode(state->IDEX.instr) == BEQ) {
			newState->EXMEM.aluResult = muxRegA - muxRegB;
		} else if (opcode(state->IDEX.instr) == JALR) {// is not accepted
			errExit("cannot run with JALR operation");
		} else if (opcode(state->IDEX.instr) == HALT) {
		} else if (opcode(state->IDEX.instr) == NOOP) {
			newState->EXMEM.aluResult = 0;
		} else {
			errExit("failed to identify operation");
		}
		newState->EXMEM.readRegB = muxRegB;

		/* --------------------- MEM stage --------------------- */
		newState->MEMWB.instr = state->EXMEM.instr;
		if (opcode(state->EXMEM.instr) == ADD) {
			newState->MEMWB.writeData = state->EXMEM.aluResult;
		} else if (opcode(state->EXMEM.instr) == NOR) {
			newState->MEMWB.writeData = state->EXMEM.aluResult;
		} else if (opcode(state->EXMEM.instr) == LW) {
			newState->MEMWB.writeData = state->dataMem[state->EXMEM.aluResult];
		} else if (opcode(state->EXMEM.instr) == SW) {
			newState->dataMem[state->EXMEM.aluResult] = state->EXMEM.readRegB;
		} else if (opcode(state->EXMEM.instr) == BEQ) {
			if (!state->EXMEM.aluResult) {// if branch was taken discard registers
				newState->pc = state->EXMEM.branchTarget;
				newState->IFID.instr = newState->IDEX.instr = newState->EXMEM.instr = NOOPINSTRUCTION;
			}
		} else if (opcode(state->EXMEM.instr) == JALR) {// is not accepted
			errExit("cannot run with JALR operation");
		} else if (opcode(state->EXMEM.instr) == HALT) {
		} else if (opcode(state->EXMEM.instr) == NOOP) {
		} else {
			errExit("failed to identify operation");
		}

		/* --------------------- WB  stage --------------------- */
		if (opcode(state->MEMWB.instr) == ADD) {
			newState->reg[field2(state->MEMWB.instr)] = state->MEMWB.writeData;
		} else if (opcode(state->MEMWB.instr) == NOR) {
			newState->reg[field2(state->MEMWB.instr)] = state->MEMWB.writeData;
		} else if (opcode(state->MEMWB.instr) == LW) {
			newState->reg[field1(state->MEMWB.instr)] = state->MEMWB.writeData;
		} else if (opcode(state->MEMWB.instr) == SW) {

		} else if (opcode(state->MEMWB.instr) == BEQ) {

		} else if (opcode(state->MEMWB.instr) == JALR) {// is not accepted
			errExit("cannot run with JALR operation");
		} else if (opcode(state->MEMWB.instr) == HALT) {

		} else if (opcode(state->MEMWB.instr) == NOOP) {

		} else {
			errExit("failed to identify operation");
		}
		newState->WBEND.instr = state->MEMWB.instr;
		newState->WBEND.writeData = state->MEMWB.writeData;

		/* --------------------- END stage --------------------- */
		memcpy(state, newState, sizeof(stateType));
		/* this is the last statement before end of the loop.
				It marks the end of the cycle and updates the
				current state with the values calculated in this
				cycle */
	}
}

void initState(stateType *state)
{
	memset(state, 0, sizeof(stateType));
	state->IFID.instr = NOOPINSTRUCTION;
	state->IDEX.instr = NOOPINSTRUCTION;
	state->EXMEM.instr = NOOPINSTRUCTION;
	state->MEMWB.instr = NOOPINSTRUCTION;
	state->WBEND.instr = NOOPINSTRUCTION;
}

// If previous instruction was LW and dest reg of LW instruction is
// used by current instruction, stall one cycle to prevent data hazard.
bool isHazard(stateType *state)
{
	int curInstr = opcode(state->IFID.instr);
	if (opcode(state->IDEX.instr) == LW) {
		int destReg = field1(state->IDEX.instr);
		int regA = field0(state->IFID.instr);
		int regB = field1(state->IFID.instr);
		if (curInstr == ADD || curInstr == NOR
			|| curInstr == BEQ) {
			return (destReg == regA || destReg == regB);
		}
		else if (curInstr == LW || curInstr == SW) {
			return (destReg == regA);
		}
	}

	return false;
}

// This function will forward data to EX stage if necessary.
void forwardRegOnHazard(stateType *state, int *inp1, int *inp2)
{
	int curInstr = opcode(state->IDEX.instr);
	int regA = field0(state->IDEX.instr), regB = field1(state->IDEX.instr);
	if (curInstr == JALR || curInstr == HALT || curInstr == NOOP)
		return;
	int prevInstr, destReg;
	prevInstr = opcode(state->WBEND.instr);
	if (prevInstr == LW) {
		destReg = field1(state->WBEND.instr);
		if (regA == destReg)
			*inp1 = state->WBEND.writeData;
		if (regB == destReg)
			*inp2 = state->WBEND.writeData;
	}
	else if (prevInstr == ADD || prevInstr == NOR) {
		destReg = field2(state->WBEND.instr);
		if (regA == destReg)
			*inp1 = state->WBEND.writeData;
		if (regB == destReg)
		    *inp2 = state->WBEND.writeData;
	}

	prevInstr = opcode(state->MEMWB.instr);
	if (prevInstr == LW) {
		destReg = field1(state->MEMWB.instr);
		if (regA == destReg)
			*inp1 = state->MEMWB.writeData;
		if (regB == destReg)
			*inp2 = state->MEMWB.writeData;
	}
	else if (prevInstr == ADD || prevInstr == NOR) {
		destReg = field2(state->MEMWB.instr);
		if (regA == destReg)
			*inp1 = state->MEMWB.writeData;
		if (regB == destReg)
			*inp2 = state->MEMWB.writeData;
	}

	prevInstr = opcode(state->EXMEM.instr);
	if (prevInstr == ADD || prevInstr == NOR) {
		destReg = field2(state->EXMEM.instr);
		if (regA == destReg)
			*inp1 = state->EXMEM.aluResult;
		if (regB == destReg)
			*inp2 = state->EXMEM.aluResult;
	}

}

int convertNum(int num)
{
	/* convert a 16-bit number into a 32-bit Linux integer */
	if (num & (1<<15) ) {
		num -= (1<<16);
	}
	return num;
}

void printBinary(int ml)
{
	for (int i = 31; i >= 0; --i) {
		unsigned char bit = ((ml >> i) & 1);
		printf("%u", bit);
		if (i % 8 == 0)
			printf(" ");
	}
	printf("\n");
}

void errExit(char *str)
{
	fprintf(stderr, "ERROR: %s\n", str);
	exit(1);
}


void
printState(stateType *statePtr)
{
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
	printf("\tpc %d\n", statePtr->pc);

	printf("\tdata memory:\n");
	for (i=0; i<statePtr->numMemory; i++) {
		printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}
	printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
	printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
	printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX.offset);
	printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM.instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
	printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
	printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

int
field0(int instruction)
{
	return( (instruction>>19) & 0x7);
}

int
field1(int instruction)
{
	return( (instruction>>16) & 0x7);
}

int
field2(int instruction)
{
	return(instruction & 0xFFFF);
}

int
opcode(int instruction)
{
	return(instruction>>22);
}

void
printInstruction(int instr)
{

	char opcodeString[10];

	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	} else if (opcode(instr) == NOR) {
		strcpy(opcodeString, "nor");
	} else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	} else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	} else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	} else if (opcode(instr) == JALR) {
		strcpy(opcodeString, "jalr");
	} else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	} else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	} else {
		strcpy(opcodeString, "data");
	}
	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
		field2(instr));
}
