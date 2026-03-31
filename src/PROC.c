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

	uint32_t eff_addr;
	
	

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);

		//Print contents of the register file after each instruction
		
		printRegFile();//only suggested for Debug, comment this line to reduce output
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/
		CurrentInstruction = readWord(ProgramCounter, false);

		opcode = (CurrentInstruction >> 26) & 0x3F;
		rs = (CurrentInstruction >> 21) & 0x1F;
		rt = (CurrentInstruction >> 16) & 0x1F;
		rd = (CurrentInstruction >> 11) & 0x1F;
		shamt = (CurrentInstruction >> 6) & 0x1F;
		funct = CurrentInstruction & 0x3F;
		imm = CurrentInstruction & 0xFFFF;
		addr = CurrentInstruction & 0x03FFFFFF;

		simm = (int32_t)(int16_t)imm;
		nextPC = ProgramCounter + 4;

		u_rs = RegFile[rs];
		u_rt = RegFile[rt];
		s_rs = (int32_t)RegFile[rs];
		s_rt = (int32_t)RegFile[rt];

		switch(opcode) {
			/**********************************************************/
    		/* LOADS AND STORES----CHLOE LIU                          */
    		/**********************************************************/
			case 0x20:{ // LB
				eff_addr = u_rs + simm; //calculate addresss
				uint8_t val = readByte(eff_addr,false);
				RegFile[rt] = (int32_t)(int8_t)val; // zero extend and load
				break;
			}
			case 0x21: {  // LH
				eff_addr = u_rs + simm; //calculate address
				uint8_t byte1,byte2;
				byte1 = readByte(eff_addr,false);
				byte2 = readByte(eff_addr+1,false);
				uint16_t val =(byte1 << 8) | byte2; //adding the bytes to form the half word
				RegFile[rt] = (int32_t)(int16_t)val; // zero-extent and load
				break;
			}
			case 0x22: {  // LWL
				eff_addr = u_rs + simm; //calculate addresss
				uint32_t offset_addr, val, offset;
				offset_addr = eff_addr & ~0x3;
				val = readWord(offset_addr,false);
				offset = eff_addr & 0x3;
				// different merging situations offset=0 to offset=3
				if (offset == 0){
					RegFile[rt] = val;
				}
				else if (offset == 1) {
					RegFile[rt] = (val & 0xFFFFFF00) | (RegFile[rt] & 0x000000FF);
				}
				else if (offset == 2) {
					RegFile[rt] = (val & 0xFFFF0000) | (RegFile[rt] & 0x0000FFFF);
				}
				else if (offset == 3) {
					RegFile[rt] = (val & 0xFF000000) | (RegFile[rt] & 0x00FFFFFF);
				}
				break;
			}
			case 0x23: {  // LW
				eff_addr = u_rs + simm; //calculate address
				RegFile[rt] = readWord(eff_addr,false); //load
				break;
			}
			case 0x24:   // LBU
			
				// 1. Compute effective address
				// 2. Read 1 byte from memory
				// 3. Zero-extend the byte to 32 bits
				// 4. Store the result into RegFile[rt]
				break;

			case 0x25:   // LHU
				// 1. Compute effective address
				// 2. Read 2 bytes (halfword) from memory
				// 3. Combine them into a 16-bit value using big-endian order
				// 4. Zero-extend the halfword to 32 bits
				// 5. Store the result into RegFile[rt]
				break;

			case 0x26:   // LWR
				// 1. Compute effective address
				// 2. Find the aligned word boundary containing that address
				// 3. Read the full word from memory
				// 4. Use the low 2 bits of the address to determine how many right-side bytes to load
				// 5. Merge those bytes into the lower part of RegFile[rt]
				// 6. Keep the remaining upper bytes of RegFile[rt] unchanged
				break;

			case 0x28:   // SB
				// 1. Compute effective address
				// 2. Take the lowest 8 bits of RegFile[rt]
				// 3. Write that byte into memory
				break;

			case 0x29:   // SH
				// 1. Compute effective address
				// 2. Take the lowest 16 bits of RegFile[rt]
				// 3. Split them into 2 bytes using big-endian order
				// 4. Write those bytes into memory
				break;

			case 0x2A:   // SWL
				// 1. Compute effective address
				// 2. Find the aligned word boundary containing that address
				// 3. Take the upper bytes from RegFile[rt]
				// 4. Use the low 2 bits of the address to determine how many left-side bytes to store
				// 5. Write those bytes into memory
				break;

			case 0x2B:   // SW
				// 1. Compute effective address
				// 2. Take the full 32-bit value from RegFile[rt]
				// 3. Write the full word into memory
				break;

			case 0x2E:   // SWR
				// 1. Compute effective address
				// 2. Find the aligned word boundary containing that address
				// 3. Take the lower bytes from RegFile[rt]
				// 4. Use the low 2 bits of the address to determine how many right-side bytes to store
				// 5. Write those bytes into memory
				break;

			/**********************************************************/
    		/* ALU -------Ulizes                                     */
    		/**********************************************************/

			/**********************************************************/
    		/* SHIFTS-----CHLOE LIU                                   */
    		/**********************************************************/

			/**********************************************************/
    		/* MULTI AND DIVISION----Ulizes                           */
    		/**********************************************************/

			/**********************************************************/
    		/* JUMP AND BRANCH------CHLOE LIU                         */
    		/**********************************************************/

			/**********************************************************/
    		/* EXCEPTION-----Ulizes                                   */
    		/**********************************************************/

			default:
				break;
		}

		RegFile[0] = 0;
		ProgramCounter = nextPC;
	


	}   
	//Print the final contents of the register file
	printRegFile();
	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;

}
