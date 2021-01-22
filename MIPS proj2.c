#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BITMASK(n) ((1 << n) - 1)

#define Type_R_MIPS 'R'
#define Type_J_MIPS 'J'
#define Type_I_MIPS 'I'

#define Root_Type_Jump_Or_Branch_MIPS 0
#define Root_Type_Arithlogi_MIPS 1
#define Root_Type_LoadI_Or_Trap_MIPS 3
#define Root_Type_Loadstore_GTE_MIPS 4

#define Reg_Type_Shift_Or_Shiftv_MIPS 0
#define Reg_Type_Jumpr_MIPS 1
#define Reg_Type_Move_MIPS 2
#define Reg_Type_Divmult_MIPS 3
#define Reg_Type_Arithlog_Gte_MIPS 4


typedef struct {
    char* name;
    char type;
    char arguments[32];
} mips_instruction_t;


int reg[32] = { 0, };
int pcreg = 0;
int ck = 0;
int jck = 0;
unsigned int datamem[16384];



static const char* const Register_Names_MIPS[32] = {
    "$0",
    "$1",
    "$2",
    "$3",
    "$4",
    "$5",
    "$6",
    "$7",
    "$8",
    "$9",
    "$10",
    "$11",
    "$12",
    "$13",
    "$14",
    "$15",
    "$16",
    "$17",
    "$18",
    "$19",
    "$20",
    "$21",
    "$22",
    "$23",
    "$24",
    "$25",
    "$26",
    "$27",
    "$28",
    "$29",
    "$30",
    "$31"
};


static char* const Register_Instruction_Names_MIPS[8][8] = {
    {"sll", NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {"add", NULL, "sub", NULL, "and", "or", NULL, NULL},
    {NULL, NULL, "slt", "sltu"}
};

static char* const Root_Instruction_Names_MIPS[8][8] = {
    {NULL, NULL, "j", NULL, "beq", "bne", NULL, NULL},
    {"addi", NULL, "slti", "sltiu", "andi", "ori", NULL, "lui"},
    {NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL, "lw", NULL, NULL, NULL},
    {NULL, NULL, NULL, "sw", NULL, NULL, NULL},
    {NULL},
    {NULL}
};



int mips_disassemble(mips_instruction_t* instruction, uint32_t number) {

    uint8_t op, op_lower, op_upper,
        rs, rd,
        rt, rt_upper, rt_lower,
        sa,
        funct, funct_upper, funct_lower;
    int16_t imm;
    int32_t target;

    op = number >> 26;
    op_upper = (op >> 3) & BITMASK(3);
    op_lower = op & BITMASK(3);

    rs = (number >> 21) & BITMASK(5);

    rt = (number >> 16) & BITMASK(5);
    rt_upper = rt >> 3;
    rt_lower = rt & BITMASK(3);

    rd = (number >> 11) & BITMASK(5);
    sa = (number >> 6) & BITMASK(5);

    funct = number & BITMASK(6);
    funct_upper = (funct >> 3) & BITMASK(3);
    funct_lower = funct & BITMASK(3);

    imm = number & BITMASK(16);

    target = number & BITMASK(26);
    if (target > BITMASK(25)) {
        target |= 0xfc000000;
    }

    if (op == 0) {
        instruction->name =
            Register_Instruction_Names_MIPS[funct_upper][funct_lower];
        instruction->type = Type_R_MIPS;
    }
    else {
        instruction->name =
            Root_Instruction_Names_MIPS[op_upper][op_lower];
        instruction->type = Type_I_MIPS;
    }


    if (instruction->name == NULL) {
        return 0;
    }
    // if (number == 0) {
    //     instruction->name = "nop";
    //     instruction->arguments[0] = 0;

    // }
    else if (op == 0) {
        switch (funct_upper) {
        case Reg_Type_Shift_Or_Shiftv_MIPS:
            if (funct_lower < 4) { //Shift
                sprintf(instruction->arguments, "%s, %s, %d",
                    Register_Names_MIPS[rd],
                    Register_Names_MIPS[rt],
                    sa);
            }
            else { //ShiftV
                sprintf(instruction->arguments, "%s, %s, %s",
                    Register_Names_MIPS[rd],
                    Register_Names_MIPS[rt],
                    Register_Names_MIPS[rs]);
            }
            break;

        case Reg_Type_Jumpr_MIPS:
            if (funct_lower < 1) {
                sprintf(instruction->arguments, "%s", Register_Names_MIPS[rs]);
            }
            else if (funct_lower == 4) {
                sprintf(instruction->arguments, "%s", "");
            }
            else {
                sprintf(instruction->arguments, "%s, %s",
                    Register_Names_MIPS[rd],
                    Register_Names_MIPS[rs]);
            }
            break;

        case Reg_Type_Move_MIPS:
            if (funct_lower % 2 == 0) {
                sprintf(instruction->arguments, "%s", Register_Names_MIPS[rd]);
            }
            else {
                sprintf(instruction->arguments, "%s", Register_Names_MIPS[rs]);
            }
            break;

        case Reg_Type_Divmult_MIPS:
            sprintf(instruction->arguments, "%s, %s",
                Register_Names_MIPS[rs],
                Register_Names_MIPS[rt]);
            break;

        case Reg_Type_Arithlog_Gte_MIPS:
        case Reg_Type_Arithlog_Gte_MIPS + 1:
            sprintf(instruction->arguments, "%s, %s, %s",
                Register_Names_MIPS[rd],
                Register_Names_MIPS[rs],
                Register_Names_MIPS[rt]);
            break;

        default:
            return 0;
        }

    }
    else if (op == 1) {
        sprintf(instruction->arguments, "%s, %d",
            Register_Names_MIPS[rs],
            imm);


    }
    else {
        switch (op_upper) {
        case Root_Type_Jump_Or_Branch_MIPS:
            if (op_lower < 4) { // Jump
                sprintf(instruction->arguments, "%d", target);
                instruction->type = Type_J_MIPS;
            }
            else {
                if (op_lower < 6) { //Branch
                    sprintf(instruction->arguments, "%s, %s, %d",
                        Register_Names_MIPS[rs],
                        Register_Names_MIPS[rt],
                        imm);
                }
                else { //BranchZ
               
                    sprintf(instruction->arguments, "%s, %d",
                        Register_Names_MIPS[rs],
                        imm);
                }
            }
            break;

        case Root_Type_Arithlogi_MIPS:
            if (op_lower < 7) {
                sprintf(instruction->arguments, "%s, %s, %d",
                    Register_Names_MIPS[rt],
                    Register_Names_MIPS[rs],
                    imm);
            }
            else {
                sprintf(instruction->arguments, "%s, %d",
                    Register_Names_MIPS[rt],
                    imm);
            }
            break;

        case Root_Type_Loadstore_GTE_MIPS:
        case Root_Type_Loadstore_GTE_MIPS + 1:
        case Root_Type_Loadstore_GTE_MIPS + 2:
        case Root_Type_Loadstore_GTE_MIPS + 3:
            sprintf(instruction->arguments, "%s, %d(%s)",
                Register_Names_MIPS[rt],
                imm,
                Register_Names_MIPS[rs]);
            break;

        default:
            return 0;
        }
    }

    return 1;
}


int mips_assemble(mips_instruction_t* instruction, uint32_t number) {
    uint8_t op, op_lower, op_upper,
        rs, rd, rt, rt_upper, rt_lower, sa, funct, funct_upper, funct_lower;
    uint16_t imme;
    int32_t target;
    int16_t imm, offset;

    int rand;
    unsigned int randu;

    op = number >> 26;
    op_upper = (op >> 3) & BITMASK(3);
    op_lower = op & BITMASK(3);

    rs = (number >> 21) & BITMASK(5);
    rt = (number >> 16) & BITMASK(5);
    rt_upper = rt >> 3;
    rt_lower = rt & BITMASK(3);
    rd = (number >> 11) & BITMASK(5);
    sa = (number >> 6) & BITMASK(5);

    funct = number & BITMASK(6);
    funct_upper = (funct >> 3) & BITMASK(3);
    funct_lower = funct & BITMASK(3);

    imm = number & BITMASK(16);
    imme = number & BITMASK(16);
    offset = number & BITMASK(16);

    target = number & BITMASK(26);
    if (target > BITMASK(25)) {
        target |= 0xfc000000;
    }
    if (op == 0) {
        instruction->name =
            Register_Instruction_Names_MIPS[funct_upper][funct_lower];
        instruction->type = Type_R_MIPS;
    }
    else {
        instruction->name =
            Root_Instruction_Names_MIPS[op_upper][op_lower];
        instruction->type = Type_I_MIPS;
    }
    if (instruction->name == NULL) {
        return 0;
    }

    else {
        if (instruction->type == Type_R_MIPS) {
            if (sa == 0) {
                if (funct == 32) { //add
                    reg[rd] = reg[rs] + reg[rt];
                    return 1;
                }

                else if (funct == 34) { //sub
                    reg[rd] = reg[rs] - reg[rt];
                    return 1;
                }

                else if (funct == 36) { //and
                    reg[rd] = reg[rs] & reg[rt];
                    return 1;
                }

                else if (funct == 37) { //or
                    reg[rd] = reg[rs] | reg[rt];
                    return 1;
                }

                else if (funct == 42) { //slt
                    reg[rd] = (reg[rs] < reg[rt]) ? 1 : 0;
                    return 1;
                }

                else if (funct == 43) { //sltu
                    reg[rd] = ((unsigned int)reg[rs] < reg[rt]) ? 1 : 0;
                    return 1;
                }
                else // sll
                    return 1;
            }
        }
        else {
            if (op == 2) { // j
                pcreg = (((pcreg >> 28) & BITMASK(4)) << 28) | (target << 2);
                jck++;
                return 1;
            }
            else if (op == 8) { //addi
                reg[rt] = reg[rs] + imm;
                return 1;
            }

            else if (op == 4) { // beq
                if (reg[rs] == reg[rt])
                    pcreg = pcreg + (offset * 4);
                return 1;
            }

            else if (op == 5) { // bne
                if (reg[rs] != reg[rt])
                    pcreg = pcreg + (offset * 4);
                return 1;
            }

            else if (op == 10) { //slti
                rand = imm;
                reg[rt] = (reg[rs] < rand) ? 1 : 0;
                return 1;
            }

            else if (op == 11) { //sltiu
                rand = imm;
                reg[rt] = (reg[rs] < (unsigned int)rand) ? 1 : 0;
                return 1;
            }

            else if (op == 12) { //andi
                reg[rt] = reg[rs] & imm;
                return 1;
            }

            else if (op == 13) { //ori
                reg[rt] = reg[rs] | imme;
                return 1;
            }

            else if (op == 15) { //lui
                reg[rt] = (imm << 16); 
                return 1;
            }

            else if (op == 35) { //lw
                reg[rt] = datamem[(reg[rs] + offset - 0x10000000) / 4];
                return 1;
            }

            else { //sw
                datamem[(reg[rs] + offset - 0x10000000) / 4] = reg[rt];
                return 1;

            }
        }

    }

    return 1;
}


int main(int argc, char*argv[]) // int argc char*argv[] 
{
    FILE* fp;

    int c;
    int line_number = 0;
    char s1[10];
    char buffer[4] = { 0, };
    int instmem[16384];
 
    uint32_t number;
    int cw;
    int addr = 0;

  
    fp = fopen(argv[1], "rb");  
    memset(instmem, 0xFF, sizeof(instmem));
    memset(datamem, 0xFF, sizeof(datamem));

    while (!feof(fp)) {
        fread(buffer, sizeof(buffer), 1, fp);
        if (feof(fp)) break;

        int rtr = ((unsigned char)buffer[0] << 24) | ((unsigned char)buffer[1] << 16) | ((unsigned char)buffer[2] << 8) | ((unsigned char)buffer[3] << 0);
        instmem[line_number] = rtr;
        line_number += 1;

    }

    if(argc<=3)
    {
        for (cw = 0; cw < atoi(argv[2]); cw++)
        {
            number = instmem[cw];
            mips_instruction_t* instruction = calloc(1, sizeof(mips_instruction_t));
            if (!(mips_assemble(instruction, number))) {
                printf("unknown instruction\n");
                break;
            }
        }
    }
    else
    {
    if (!(strcmp(argv[3], "mem")))
    {
        
        for (cw = 0; cw < atoi(argv[2]); cw++)
        {
            number = instmem[cw];
            mips_instruction_t* instruction = calloc(1, sizeof(mips_instruction_t));
            if (!(mips_assemble(instruction, number))) {
                printf("unknown instruction\n");
                break;
            }
        }
        for (int i = 0; i < atoi(argv[5]); i++)
        {
            printf("0x%08x\n", datamem[((atoi(argv[4]) + (4 * i)) - 0x10000000) / 4]);
        }
    }
    else if (!(strcmp(argv[3], "reg")))
    {
        for (cw = 0; cw < atoi(argv[2]); cw++)
        {
            number = instmem[(pcreg/4)];
            mips_instruction_t* instruction = calloc(1, sizeof(mips_instruction_t));
            pcreg = pcreg + 4;
            if (!(mips_assemble(instruction, number))) {
                printf("unknown instruction\n");
                break;
            }
        }
        for (int i = 0; i < 32; i++)
        {
            printf("$%d: 0x%08x\n", i, reg[i]);
        }
        printf("PC: 0x%08x\n", pcreg);
    }
    else
    {
    }
    }
    


    


    fclose(fp);


    return 0;
}




