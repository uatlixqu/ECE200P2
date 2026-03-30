#include <stdio.h>	/* fprintf(), printf() */
#include <stdlib.h>	/* atoi() */
#include <stdint.h>	/* uint32_t */

#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

int main(int argc, char * argv[]) {

	/*
	 * This variable will store the maximum
	 * number of instructions to run before
	 * forcibly terminating the program. It
	 * is set via a command line argument.
	 */
	uint32_t MaxInstructions;

	/*
	 * This variable will store the address
	 * of the next instruction to be fetched
	 * from the instruction memory.
	 */
	uint32_t ProgramCounter;

	/*
	 * This variable will store the instruction
	 * once it is fetched from instruction memory.
	 */
	uint32_t CurrentInstruction;

	//IF THE USER HAS NOT SPECIFIED ENOUGH COMMAND LINE ARUGMENTS
	if(argc < 3){

		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Input argument missing!\n");
		fprintf(stderr, "Expected: file-name, max-instructions\n");
		return -1;

	}

     	//CONVERT MAX INSTRUCTIONS FROM STRING TO INTEGER	
	MaxInstructions = atoi(argv[2]);	

	//Open file pointers & initialize Heap & Regsiters
	initHeap();
	initFDT();
	initRegFile(0);

	//LOAD ELF FILE INTO MEMORY AND STORE EXIT STATUS
	int status = LoadOSMemory(argv[1]);

	//IF LOADING FILE RETURNED NEGATIVE EXIT STATUS
	if(status < 0){ 
		
		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Unable to open file at %s!\n", argv[1]);
		return status; 
	
	}

	printf("\n ----- BOOT Sequence ----- \n");
	printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", exec.GSP, exec.GP, exec.GPC_START);

	RegFile[28] = exec.GP;
	RegFile[29] = exec.GSP;
	RegFile[31] = exec.GPC_START;

	printRegFile();

	printf("\n ----- Execute Program ----- \n");
	printf("Max Instruction to run = %d \n",MaxInstructions);
	fflush(stdout);
	ProgramCounter = exec.GPC_START;
	
	/***************************/
	/* ADD YOUR VARIABLES HERE
	/***************************/
	uint32_t opcode, rs, rt, rd, shamt, funct;
	uint32_t imm, addr;
	uint32_t nextPC;
	int32_t simm;

	int64_t mult_result;
	uint64_t umult_result;
	int32_t s_rs, s_rt;
	uint32_t u_rs, u_rt;
	

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);

		//Print contents of the register file after each instruction
		
		printRegFile();//only suggested for Debug, comment this line to reduce output
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/
		for(i = 0; i < MaxInstructions; i++) {

			CurrentInstruction = readWord(ProgramCounter, false);

			opcode = (CurrentInstruction >> 26) & 0x3F;
			rs     = (CurrentInstruction >> 21) & 0x1F;
			rt     = (CurrentInstruction >> 16) & 0x1F;
			rd     = (CurrentInstruction >> 11) & 0x1F;
			shamt  = (CurrentInstruction >> 6) & 0x1F;
			funct  = CurrentInstruction & 0x3F;
			imm    = CurrentInstruction & 0xFFFF;
			addr   = CurrentInstruction & 0x03FFFFFF;

			simm   = (int32_t)(int16_t)imm;
			nextPC = ProgramCounter + 4;

			u_rs = RegFile[rs];
			u_rt = RegFile[rt];
			s_rs = (int32_t)RegFile[rs];
			s_rt = (int32_t)RegFile[rt];

			switch(opcode) {

				case 0x00:
					switch(funct) {
						case 0x20:   // add
							RegFile[rd] = s_rs + s_rt;
							break;

						case 0x22:   // sub
							RegFile[rd] = s_rs - s_rt;
							break;

						default:
							break;
					}
					break;

				case 0x08:   // addi
					RegFile[rt] = s_rs + simm;
					break;

				case 0x23:   // lw
					RegFile[rt] = readWord(u_rs + simm, false);
					break;

				case 0x2B:   // sw
					writeWord(u_rs + simm, RegFile[rt], false);
					break;

				default:
					break;
			}

			RegFile[0] = 0;
			ProgramCounter = nextPC;
		}


	}   
	//Print the final contents of the register file
	printRegFile();
	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;

}
