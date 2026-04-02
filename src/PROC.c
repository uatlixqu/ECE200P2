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

	/* IF THE USER HAS NOT SPECIFIED ENOUGH COMMAND LINE ARGUMENTS */
	if(argc < 3){
		fprintf(stderr, "ERROR: Input argument missing!\n");
		fprintf(stderr, "Expected: file-name, max-instructions\n");
		return -1;
	}

	/* CONVERT MAX INSTRUCTIONS FROM STRING TO INTEGER */
	MaxInstructions = (uint32_t)atoi(argv[2]);

	/* Open file pointers & initialize Heap & Registers */
	initHeap();
	initFDT();
	initRegFile(0);

	/* LOAD ELF FILE INTO MEMORY AND STORE EXIT STATUS */
	{
		int status = LoadOSMemory(argv[1]);
		if(status < 0){
			fprintf(stderr, "ERROR: Unable to open file at %s!\n", argv[1]);
			return status;
		}
	}

	printf("\n ----- BOOT Sequence ----- \n");
	printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", exec.GSP, exec.GP, exec.GPC_START);

	RegFile[28] = exec.GP;
	RegFile[29] = exec.GSP;
	RegFile[31] = exec.GPC_START;

	printRegFile();

	printf("\n ----- Execute Program ----- \n");
	printf("Max Instruction to run = %u \n", MaxInstructions);
	fflush(stdout);
	ProgramCounter = exec.GPC_START;

	/***************************/
	/* EXECUTION VARIABLES     */
	/***************************/
	uint32_t opcode = 0, rs = 0, rt = 0, rd = 0, shamt = 0, funct = 0;
	uint32_t imm = 0, addr = 0;
	uint32_t nextPC = 0;
	int32_t  simm = 0;

	int32_t  s_rs = 0, s_rt = 0;
	uint32_t u_rs = 0, u_rt = 0;
	uint32_t eff_addr = 0;

	/* branch delay slot support */
	uint32_t target = 0;
	int branch = 0;

	/* load delay slot support */
	int32_t  load_val = 0;
	uint32_t load_reg = 0;
	int pending = 0;

	int i;
	for(i = 0; i < (int)MaxInstructions; i++) {

		/* Fetch Current Instruction */
		CurrentInstruction = readWord(ProgramCounter, false);

		/* Decode Current Instruction */
		opcode = (CurrentInstruction >> 26) & 0x3F;
		rs     = (CurrentInstruction >> 21) & 0x1F;
		rt     = (CurrentInstruction >> 16) & 0x1F;
		rd     = (CurrentInstruction >> 11) & 0x1F;
		shamt  = (CurrentInstruction >>  6) & 0x1F;
		funct  =  CurrentInstruction        & 0x3F;
		imm    =  CurrentInstruction        & 0xFFFF;
		addr   =  CurrentInstruction        & 0x03FFFFFF;

		simm   = (int32_t)(int16_t)imm;
		nextPC = ProgramCounter + 4;

		u_rs = (uint32_t)RegFile[rs];
		u_rt = (uint32_t)RegFile[rt];
		s_rs = (int32_t)RegFile[rs];
		s_rt = (int32_t)RegFile[rt];

		/* Save previous pending branch/load so current instruction becomes the delay-slot instruction. */
		int delay_branch = branch;
		uint32_t delay_target = target;
		branch = 0;

		int apply_load = pending;
		uint32_t apply_load_reg = load_reg;
		int32_t apply_load_value = load_val;
		pending = 0;

		switch(opcode) {
			/* R-TYPE INSTRUCTIONS */
			case 0x00: {
				switch(funct) {
					/* R-TYPE ALU */
					case 0x20: /* ADD */
						if(rd != 0) RegFile[rd] = s_rs + s_rt;
						break;
					case 0x21: /* ADDU */
						if(rd != 0) RegFile[rd] = RegFile[rs] + RegFile[rt];
						break;
					case 0x22: /* SUB */
						if(rd != 0) RegFile[rd] = s_rs - s_rt;
						break;
					case 0x23: /* SUBU */
						if(rd != 0) RegFile[rd] = RegFile[rs] - RegFile[rt];
						break;
					case 0x24: /* AND */
						if(rd != 0) RegFile[rd] = RegFile[rs] & RegFile[rt];
						break;
					case 0x25: /* OR */
						if(rd != 0) RegFile[rd] = RegFile[rs] | RegFile[rt];
						break;
					case 0x26: /* XOR */
						if(rd != 0) RegFile[rd] = RegFile[rs] ^ RegFile[rt];
						break;
					case 0x27: /* NOR */
						if(rd != 0) RegFile[rd] = ~(RegFile[rs] | RegFile[rt]);
						break;
					case 0x2A: /* SLT */
						if(rd != 0) RegFile[rd] = (s_rs < s_rt) ? 1 : 0;
						break;
					case 0x2B: /* SLTU */
						if(rd != 0) RegFile[rd] = (u_rs < u_rt) ? 1 : 0;
						break;

					/* R-TYPE SHIFTS */
					case 0x00: /* SLL */
						if(rd != 0) RegFile[rd] = (int32_t)(u_rt << shamt);
						break;
					case 0x02: /* SRL */
						if(rd != 0) RegFile[rd] = (int32_t)(u_rt >> shamt);
						break;
					case 0x03: /* SRA */
						if(rd != 0) RegFile[rd] = s_rt >> shamt;
						break;
					case 0x04: /* SLLV */
						if(rd != 0) RegFile[rd] = (int32_t)(u_rt << (u_rs & 0x1F));
						break;
					case 0x06: /* SRLV */
						if(rd != 0) RegFile[rd] = (int32_t)(u_rt >> (u_rs & 0x1F));
						break;
					case 0x07: /* SRAV */
						if(rd != 0) RegFile[rd] = s_rt >> (u_rs & 0x1F);
						break;

					/* R-TYPE MULTIPLICATION AND DIVISION */
					case 0x10: /* MFHI */
						if(rd != 0) RegFile[rd] = RegFile[32];
						break;
					case 0x11: /* MTHI */
						RegFile[32] = RegFile[rs];
						break;
					case 0x12: /* MFLO */
						if(rd != 0) RegFile[rd] = RegFile[33];
						break;
					case 0x13: /* MTLO */
						RegFile[33] = RegFile[rs];
						break;
					case 0x18: { /* MULT */
						int64_t result = (int64_t)s_rs * (int64_t)s_rt;
						RegFile[32] = (int32_t)(result >> 32);
						RegFile[33] = (int32_t)result;
						break;
					}
					case 0x19: { /* MULTU */
						uint64_t result = (uint64_t)u_rs * (uint64_t)u_rt;
						RegFile[32] = (int32_t)(result >> 32);
						RegFile[33] = (int32_t)result;
						break;
					}
					case 0x1A: /* DIV */
						if(RegFile[rt] != 0) {
							RegFile[33] = s_rs / s_rt;
							RegFile[32] = s_rs % s_rt;
						}
						break;
					case 0x1B: /* DIVU */
						if(RegFile[rt] != 0) {
							RegFile[33] = (int32_t)(u_rs / u_rt);
							RegFile[32] = (int32_t)(u_rs % u_rt);
						}
						break;

					/* R-TYPE JUMP AND BRANCH */
					case 0x08: /* JR */
						branch = 1;
						target = u_rs;
						break;
					case 0x09: /* JALR */
						if(rd == 0) {
							RegFile[31] = (int32_t)(ProgramCounter + 8);
						} else {
							RegFile[rd] = (int32_t)(ProgramCounter + 8);
						}
						branch = 1;
						target = u_rs;
						break;

					/* EXCEPTIONS */
					case 0x0C: /* SYSCALL */
						SyscallExe((uint32_t)RegFile[2]);
						break;
					case 0x0D: /* BREAK */
						printf("BREAK INSTRUCTION - 0x%08x\n", CurrentInstruction);
						printRegFile();
						closeFDT();
						CleanUp();
						return 0;

					default:
						fprintf(stderr, "ERROR: Unrecognized funct 0x%02x at PC=0x%08x\n", funct, ProgramCounter);
						i = (int)MaxInstructions;
						break;
				}
				break;
			}

			/* I-TYPE LOADS AND STORES */
			case 0x20: { /* LB */
				uint8_t val;
				eff_addr = u_rs + (uint32_t)simm;
				val = readByte(eff_addr, false);
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)(int8_t)val;
				break;
			}
			case 0x21: { /* LH */
				uint8_t byte1, byte2;
				uint16_t val;
				eff_addr = u_rs + (uint32_t)simm;
				byte1 = readByte(eff_addr, false);
				byte2 = readByte(eff_addr + 1, false);
				val = (uint16_t)(((uint16_t)byte1 << 8) | (uint16_t)byte2);
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)(int16_t)val;
				break;
			}
			case 0x22: { /* LWL */
				uint32_t aligned_addr, val, offset, old_rt, result;
				eff_addr = u_rs + (uint32_t)simm;
				aligned_addr = eff_addr & ~0x3u;
				val = readWord(aligned_addr, false);
				offset = eff_addr & 0x3u;
				old_rt = (uint32_t)RegFile[rt];
				if(offset == 0)      result = val;
				else if(offset == 1) result = (val << 8)  | (old_rt & 0x000000FFu);
				else if(offset == 2) result = (val << 16) | (old_rt & 0x0000FFFFu);
				else                 result = (val << 24) | (old_rt & 0x00FFFFFFu);
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)result;
				break;
			}
			case 0x23: /* LW */
				eff_addr = u_rs + (uint32_t)simm;
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)readWord(eff_addr, false);
				break;
			case 0x24: { /* LBU */
				uint8_t val;
				eff_addr = u_rs + (uint32_t)simm;
				val = readByte(eff_addr, false);
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)(uint32_t)val;
				break;
			}
			case 0x25: { /* LHU */
				uint8_t byte1, byte2;
				uint16_t val;
				eff_addr = u_rs + (uint32_t)simm;
				byte1 = readByte(eff_addr, false);
				byte2 = readByte(eff_addr + 1, false);
				val = (uint16_t)(((uint16_t)byte1 << 8) | (uint16_t)byte2);
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)(uint32_t)val;
				break;
			}
			case 0x26: { /* LWR */
				uint32_t aligned_addr, val, offset, old_rt, result;
				eff_addr = u_rs + (uint32_t)simm;
				aligned_addr = eff_addr & ~0x3u;
				val = readWord(aligned_addr, false);
				offset = eff_addr & 0x3u;
				old_rt = (uint32_t)RegFile[rt];
				if(offset == 0)      result = (old_rt & 0xFFFFFF00u) | (val >> 24);
				else if(offset == 1) result = (old_rt & 0xFFFF0000u) | (val >> 16);
				else if(offset == 2) result = (old_rt & 0xFF000000u) | (val >> 8);
				else                 result = val;
				pending = 1;
				load_reg = rt;
				load_val = (int32_t)result;
				break;
			}
			case 0x28: { /* SB */
				uint8_t val;
				eff_addr = u_rs + (uint32_t)simm;
				val = (uint8_t)(RegFile[rt] & 0xFF);
				writeByte(eff_addr, val, false);
				break;
			}
			case 0x29: { /* SH */
				uint16_t half_val;
				uint8_t byte1, byte2;
				eff_addr = u_rs + (uint32_t)simm;
				half_val = (uint16_t)(RegFile[rt] & 0xFFFF);
				byte1 = (uint8_t)(half_val >> 8);
				byte2 = (uint8_t)(half_val & 0xFF);
				writeByte(eff_addr,     byte1, false);
				writeByte(eff_addr + 1, byte2, false);
				break;
			}
			case 0x2A: { /* SWL */
				uint32_t aligned_addr, offset, reg_val, mem_val, new_val;
				eff_addr = u_rs + (uint32_t)simm;
				aligned_addr = eff_addr & ~0x3u;
				offset = eff_addr & 0x3u;
				reg_val = (uint32_t)RegFile[rt];
				mem_val = readWord(aligned_addr, false);
				if(offset == 0)      new_val = reg_val;
				else if(offset == 1) new_val = (mem_val & 0xFF000000u) | (reg_val >> 8);
				else if(offset == 2) new_val = (mem_val & 0xFFFF0000u) | (reg_val >> 16);
				else                 new_val = (mem_val & 0xFFFFFF00u) | (reg_val >> 24);
				writeWord(aligned_addr, new_val, false);
				break;
			}
			case 0x2B: /* SW */
				eff_addr = u_rs + (uint32_t)simm;
				writeWord(eff_addr, (uint32_t)RegFile[rt], false);
				break;
			case 0x2E: { /* SWR */
				uint32_t aligned_addr, offset, reg_val, mem_val, new_val;
				eff_addr = u_rs + (uint32_t)simm;
				aligned_addr = eff_addr & ~0x3u;
				offset = eff_addr & 0x3u;
				reg_val = (uint32_t)RegFile[rt];
				mem_val = readWord(aligned_addr, false);
				if(offset == 0)      new_val = (mem_val & 0x00FFFFFFu) | (reg_val << 24);
				else if(offset == 1) new_val = (mem_val & 0x0000FFFFu) | (reg_val << 16);
				else if(offset == 2) new_val = (mem_val & 0x000000FFu) | (reg_val << 8);
				else                 new_val = reg_val;
				writeWord(aligned_addr, new_val, false);
				break;
			}

			/* I-TYPE ALU */
			case 0x08: /* ADDI */
				if(rt != 0) RegFile[rt] = s_rs + simm;
				break;
			case 0x09: /* ADDIU */
				if(rt != 0) RegFile[rt] = RegFile[rs] + simm;
				break;
			case 0x0A: /* SLTI */
				if(rt != 0) RegFile[rt] = (s_rs < simm) ? 1 : 0;
				break;
			case 0x0B: /* SLTIU */
				if(rt != 0) RegFile[rt] = (u_rs < (uint32_t)simm) ? 1 : 0;
				break;
			case 0x0C: /* ANDI */
				if(rt != 0) RegFile[rt] = RegFile[rs] & (uint32_t)imm;
				break;
			case 0x0D: /* ORI */
				if(rt != 0) RegFile[rt] = RegFile[rs] | (uint32_t)imm;
				break;
			case 0x0E: /* XORI */
				if(rt != 0) RegFile[rt] = RegFile[rs] ^ (uint32_t)imm;
				break;
			case 0x0F: /* LUI */
				if(rt != 0) RegFile[rt] = (int32_t)(imm << 16);
				break;

			/* I-TYPE JUMP AND BRANCH */
			case 0x01: {
				/* REGIMM uses the instruction rt field, not the contents of register rt. */
				switch(rt) {
					case 0x00: /* BLTZ */
						if(s_rs < 0) {
							branch = 1;
							target = nextPC + ((uint32_t)(simm << 2));
						}
						break;
					case 0x01: /* BGEZ */
						if(s_rs >= 0) {
							branch = 1;
							target = nextPC + ((uint32_t)(simm << 2));
						}
						break;
					case 0x10: /* BLTZAL */
						if(s_rs < 0) {
							RegFile[31] = (int32_t)(ProgramCounter + 8);
							branch = 1;
							target = nextPC + ((uint32_t)(simm << 2));
						}
						break;
					case 0x11: /* BGEZAL / BAL */
						if(s_rs >= 0) {
							RegFile[31] = (int32_t)(ProgramCounter + 8);
							branch = 1;
							target = nextPC + ((uint32_t)(simm << 2));
						}
						break;
					default:
						fprintf(stderr, "ERROR: Unrecognized REGIMM rt=0x%02x at PC=0x%08x\n", rt, ProgramCounter);
						i = (int)MaxInstructions;
						break;
				}
				break;
			}
			case 0x04: /* BEQ */
				if(RegFile[rs] == RegFile[rt]) {
					branch = 1;
					target = nextPC + ((uint32_t)(simm << 2));
				}
				break;
			case 0x05: /* BNE */
				if(RegFile[rs] != RegFile[rt]) {
					branch = 1;
					target = nextPC + ((uint32_t)(simm << 2));
				}
				break;
			case 0x06: /* BLEZ */
				if(s_rs <= 0) {
					branch = 1;
					target = nextPC + ((uint32_t)(simm << 2));
				}
				break;
			case 0x07: /* BGTZ */
				if(s_rs > 0) {
					branch = 1;
					target = nextPC + ((uint32_t)(simm << 2));
				}
				break;

			/* J-TYPE JUMPS */
			case 0x02: /* J */
				branch = 1;
				target = (nextPC & 0xF0000000u) | (addr << 2);
				break;
			case 0x03: /* JAL */
				RegFile[31] = (int32_t)(ProgramCounter + 8);
				branch = 1;
				target = (nextPC & 0xF0000000u) | (addr << 2);
				break;

			default:
				fprintf(stderr, "ERROR: Unrecognized opcode 0x%02x at PC=0x%08x\n", opcode, ProgramCounter);
				i = (int)MaxInstructions;
				break;
		}

		/* load delay slot */
		if (apply_load && apply_load_reg != 0) {
			RegFile[apply_load_reg] = apply_load_value;
		}

		/* register $zero is always 0 */
		RegFile[0] = 0;

		/* branch delay slot */
		if (delay_branch) {
			ProgramCounter = delay_target;
		} else {
			ProgramCounter = nextPC;
		}
	}

	/* Print the final contents of the register file */
	printRegFile();

	/* Close file pointers & free allocated Memory */
	closeFDT();
	CleanUp();

	return 0;
}
