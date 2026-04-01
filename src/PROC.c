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

	/***************************/
	/* END OF USER VARIABLES */
	/***************************/

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);

		//Print contents of the register file after each instruction
		
		printRegFile();//only suggested for Debug, comment this line to reduce output
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/

		/* Fetch Current Instrcutions */
		CurrentInstruction = readWord(ProgramCounter, false);

		/* Decode Current Instructions */
		opcode = (CurrentInstruction >> 26) & 0x3F;			// Determine R/I/J, type instruction
		rs = (CurrentInstruction >> 21) & 0x1F;				// R/I - Source Register Index
		rt = (CurrentInstruction >> 16) & 0x1F;				// R/I - Target Register Index
		rd = (CurrentInstruction >> 11) & 0x1F;				// R - Destination Register Index
		shamt = (CurrentInstruction >> 6) & 0x1F;			// R - Shift Amount
		funct = CurrentInstruction & 0x3F;					// R - Function Code
		imm = CurrentInstruction & 0xFFFF;					// I - Immediate Value
		addr = CurrentInstruction & 0x03FFFFFF;				// J - Jump Address 

		simm = (int32_t)(int16_t)imm;						// Sign-extended immediate for I-type instructions
		nextPC = ProgramCounter + 4;						// SAVE ADDRESS OF THE NEXT INSTRUCTION (FOR BRANCH AND JUMP CALCULATIONS)

		u_rs = RegFile[rs];									// Unsigned value of source register
		u_rt = RegFile[rt];									// Unsigned value of target register					
		s_rs = (int32_t)RegFile[rs];
		s_rt = (int32_t)RegFile[rt];

		/* */

		switch(opcode) {
			/* R-TYPE INSTRUCTIONS */
			case 0x00: {
				switch(funct) {
					/************************************************************************/
					/* R-TYPE ALU ------ ULIZES ATLIXQUENO                         			*/
					/************************************************************************/
					case 0x20: { // R-TYPE INSTRUCTION - ADD
						break;
					}
					case 0x21: { // R-TYPE INSTRUCTION - ADDU
						break;
					}
					case 0x22: { // R-TYPE INSTRUCTION - SUB
						break;
					}
					case 0x23: { // R-TYPE INSTRUCTION - SUBU
						break;
					}
					case 0x24: { // R-TYPE INSTRUCTION - AND
						break;
					}
					case 0x25: { // R-TYPE INSTRUCTION - OR
						break;
					}
					case 0x26: { // R-TYPE INSTRUCTION - XOR
						break;
					}
					case 0x27: { // R-TYPE INSTRUCTION - NOR
						break;
					}
					case 0x2A: { // R-TYPE INSTRUCTION - SLT
						break;
					}
					case 0x2B: { // R-TYPE INSTRUCTION - SLTU
						break;
					}
					
					/************************************************************************/
					/* R-TYPE SHIFTS ------ CHLOE LIU							   			*/
					/************************************************************************/
					case 0x00: { // R-TYPE INSTRUCTION - SLL
						RegFile[rd] = (int32_t)((uint32_t)RegFile[rt] << shamt);
						break;
					}
					case 0x02: { // R-TYPE INSTRUCTION - SRL
						RegFile[rd] = (int32_t)((uint32_t)RegFile[rt] >> shamt);
						break;
					}
					case 0x03: { // R-TYPE INSTRUCTION - SRA
						RegFile[rd] = RegFile[rt] >> shamt;
						break;
					}
					case 0x04: {   // SLLV
						RegFile[rd] = (int32_t)((uint32_t)RegFile[rt] << (u_rs & 0x1F));
						break;
					}
					case 0x06: { // R-TYPE INSTRUCTION - SRLV
						RegFile[rd] = (int32_t)((uint32_t)RegFile[rt] >> (u_rs & 0x1F));
						break;
					}
					case 0x07: { // R-TYPE INSTRUCTION - SRAV
						RegFile[rd] = RegFile[rt] >> (u_rs & 0x1F);
						break;
					}

					/************************************************************************/
					/* R-TYPE MULTIPLICATION AND DIVISION ------ ULIZES ATLIXQUENO 			*/
					/************************************************************************/
					case 0x10: { //R-TYPE INSTRUCTION - MFHI
						break;
					}
					case 0x11: { //R-TYPE INSTRUCTION - MTHI
						break;
					}
					case 0x12: { //R-TYPE INSTRUCTION - MFLO
						break;
					}	
					case 0x13: { //R-TYPE INSTRUCTION - MTLO
						break;
					}	
					case 0x18: { // R-TYPE INSTRUCTION - MULT
						break;
					}
					case 0x19: { // R-TYPE INSTRUCTION - MULTU
						break;
					}
					case 0x1A: { // R-TYPE INSTRUCTION - DIV
						break;
					}
					case 0x1B: { // R-TYPE INSTRUCTION - DIVU
						break;
					}

					/************************************************************************/
					/* R-TYPE JUMP AND BRANCH ------ CHLOE LIU 					   			*/
					/************************************************************************/
					case 0x08: { // R-TYPE INSTRUCTION - JR
						break;
					}
					case 0x09: { // R-TYPE INSTRUCTION - JALR
						break;
					}

				}
			}

			/************************************************************************/
    		/* I-TYPE LOADS AND STORES ------ CHLOE LIU                          	*/
    		/************************************************************************/
			case 0x20: { // I-TYPE INSTRUCTION - LB
				eff_addr = u_rs + simm; //calculate addresss
				uint8_t val = readByte(eff_addr,false);
				RegFile[rt] = (int32_t)(int8_t)val; // zero extend and load
				break;
			}
			case 0x21: { // I-TYPE INSTRUCTION - LH
				eff_addr = u_rs + simm; //calculate address
				uint8_t byte1,byte2;
				byte1 = readByte(eff_addr,false);
				byte2 = readByte(eff_addr+1,false);
				uint16_t val =(byte1 << 8) | byte2; //adding the bytes to form the half word
				RegFile[rt] = (int32_t)(int16_t)val; // zero-extent and load
				break;
			}
			case 0x22: {   // LWL
				uint32_t aligned_addr, val, offset;
				eff_addr = u_rs + simm;
				aligned_addr = eff_addr & ~0x3;
				val = readWord(aligned_addr, false);
				offset = eff_addr & 0x3;

				if (offset == 0) {
					RegFile[rt] = (int32_t)val;
				}
				else if (offset == 1) {
					RegFile[rt] = (int32_t)((val << 8) | ((uint32_t)RegFile[rt] & 0x000000FF));
				}
				else if (offset == 2) {
					RegFile[rt] = (int32_t)((val << 16) | ((uint32_t)RegFile[rt] & 0x0000FFFF));
				}
				else if (offset == 3) {
					RegFile[rt] = (int32_t)((val << 24) | ((uint32_t)RegFile[rt] & 0x00FFFFFF));
				}

				break;
			}
			case 0x23: { // I-TYPE INSTRUCTION - LW
				eff_addr = u_rs + simm; //calculate address
				RegFile[rt] = readWord(eff_addr,false); //load
				break;
			}
			case 0x24: { // I-TYPE INSTRUCTION - LBU
				eff_addr = u_rs + simm; //calculate addresss
				uint8_t val = readByte(eff_addr,false);
				RegFile[rt] = (int32_t)val; // zero extend and load
				break;
			}
			case 0x25: { // I-TYPE INSTRUCTION - LHU
				eff_addr = u_rs + simm; //calculate address
				uint8_t byte1,byte2;
				byte1 = readByte(eff_addr,false);
				byte2 = readByte(eff_addr+1,false);
				uint16_t val =(byte1 << 8) | byte2; //adding the bytes to form the half word
				RegFile[rt] = (int32_t)val; // zero-extent and load
				break;
			}
			case 0x26: {   // LWR
				uint32_t aligned_addr, val, offset;
				eff_addr = u_rs + simm;
				aligned_addr = eff_addr & ~0x3;
				val = readWord(aligned_addr, false);
				offset = eff_addr & 0x3;

				if (offset == 0) {
					RegFile[rt] = (int32_t)(((uint32_t)RegFile[rt] & 0xFFFFFF00) | (val >> 24));
				}
				else if (offset == 1) {
					RegFile[rt] = (int32_t)(((uint32_t)RegFile[rt] & 0xFFFF0000) | (val >> 16));
				}
				else if (offset == 2) {
					RegFile[rt] = (int32_t)(((uint32_t)RegFile[rt] & 0xFF000000) | (val >> 8));
				}
				else if (offset == 3) {
					RegFile[rt] = (int32_t)val;
				}

				break;
			}
			case 0x28: { // I-TYPE INSTRUCTION - SB
				uint8_t val;
				eff_addr = u_rs + simm;
				val = (uint8_t)(RegFile[rt] & 0xFF);
				writeByte(eff_addr, val, false);

				break;

			}
			case 0x29: {   // SH
				uint16_t half_val;
				uint8_t byte1, byte2;

				eff_addr = u_rs + simm;
				half_val = (uint16_t)(RegFile[rt] & 0xFFFF);

				byte1 = (uint8_t)(half_val >> 8);
				byte2 = (uint8_t)(half_val & 0xFF);

				writeByte(eff_addr, byte1, false);
				writeByte(eff_addr + 1, byte2, false);

				break;
			}
			case 0x2A: {   // SWL
				uint32_t aligned_addr, offset, reg_val, mem_val, new_val;

				eff_addr = u_rs + simm;
				aligned_addr = eff_addr & ~0x3;
				offset = eff_addr & 0x3;

				reg_val = (uint32_t)RegFile[rt];
				mem_val = readWord(aligned_addr, false);

				if (offset == 0) {
					new_val = reg_val;
				}
				else if (offset == 1) {
					new_val = (mem_val & 0xFF000000) | (reg_val >> 8);
				}
				else if (offset == 2) {
					new_val = (mem_val & 0xFFFF0000) | (reg_val >> 16);
				}
				else {   // offset == 3
					new_val = (mem_val & 0xFFFFFF00) | (reg_val >> 24);
				}

				writeWord(aligned_addr, new_val, false);
				break;
			}
			case 0x2B: {   // SW
				eff_addr = u_rs + simm;
				writeWord(eff_addr, (uint32_t)RegFile[rt], false);
				break;
			}
			case 0x2E: {   // SWR
				uint32_t aligned_addr, offset, reg_val, mem_val, new_val;

				eff_addr = u_rs + simm;
				aligned_addr = eff_addr & ~0x3;
				offset = eff_addr & 0x3;

				reg_val = (uint32_t)RegFile[rt];
				mem_val = readWord(aligned_addr, false);

				if (offset == 0) {
					new_val = (mem_val & 0x00FFFFFF) | (reg_val << 24);
				}
				else if (offset == 1) {
					new_val = (mem_val & 0x0000FFFF) | (reg_val << 16);
				}
				else if (offset == 2) {
					new_val = (mem_val & 0x000000FF) | (reg_val << 8);
				}
				else {   // offset == 3
					new_val = reg_val;
				}

				writeWord(aligned_addr, new_val, false);
				break;
			}
			
			/************************************************************************/
    		/* I-TYPE ALU ------ ULIZES ATLIXQUENO 									*/
    		/************************************************************************/
			case 0x08: { // I-TYPE INSTRUCTION - ADDI
				break;
			}
			case 0x09: { // I-TYPE INSTRUCTION - ADDIU
				break;
			}
			case 0x0A: { // I-TYPE INSTRUCTION - SLTI
				break;
			}
			case 0x0B: { // I-TYPE INSTRUCTION - SLTIU
				break;
			}
			case 0x0C: { // I-TYPE INSTRUCTION - ANDI
				break;
			}
			case 0x0D: { // I-TYPE INSTRUCTION - ORI
				break;
			}
			case 0x0E: { // I-TYPE INSTRUCTION - XORI
				break;
			}
			case 0x0F: { // I-TYPE INSTRUCTION - LUI
				break;
			}

			/************************************************************************/
			/* I-TYPE JUMP AND BRANCH ------ CHLOE LIU 					   			*/
			/************************************************************************/
			case 0x01: { // I-TYPE JUMP AND BRANCH INSTRUCTIONS  
				switch(u_rt) {
					case 0x00: { // I-TYPE INSTRUCTION - BLTZ
						break;
					}
					case 0x01: { // I-TYPE INSTRUCTION - BGEZ
						break;
					}
					case 0x10: { // I-TYPE INSTRUCTION - BLTZAL
						break;
					}
					case 0x11: { // I-TYPE INSTRUCTION - BGEZAL
						break;
					}
				}
				break;
			}
			case 0x04: { // I-TYPE INSTRUCTION - BEQ
				
				break;
			}
			case 0x05: { // I-TYPE INSTRUCTION - BNE
				break;
			}
			case 0x06: { // I-TYPE INSTRUCTION - BLEZ
				break;
			}
			case 0x07: { // I-TYPE INSTRUCTION - BGTZ
				break;
			}

			/************************************************************************/
			/* J-TYPE JUMP AND BRANCH ------ CHLOE LIU 					   			*/
			/************************************************************************/
			case 0x02: { // J-TYPE INSTRUCTION - J
				break;
			}
			case 0x03: { // J-TYPE INSTRUCTION - JAL
				break;
			}

			/************************************************************************/
    		/* EXCEPTION ------ ULIZES ATLIXQUENO                     				*/
    		/************************************************************************/
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
