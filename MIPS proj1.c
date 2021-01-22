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
    {"sll", NULL, "srl", "sra", "sllv", NULL, "srlv", "srav"},
    {"jr", "jalr", NULL, NULL, "syscall"},
    {"mfhi", "mthi", "mflo", "mtlo"},
    {"mult", "multu", "div", "divu"},
    {"add", "addu", "sub", "subu", "and", "or", "xor", "nor"},
    {NULL, NULL, "slt", "sltu"}
};

static char* const Root_Instruction_Names_MIPS[8][8] = {
    {NULL, NULL, "j", "jal", "beq", "bne", NULL, NULL},
    {"addi", "addiu", "slti", "sltiu", "andi", "ori", "xori", "lui"},
    {NULL},
    {NULL, NULL, NULL},
    {"lb", "lh", NULL, "lw", "lbu", "lhu", NULL},
    {"sb", "sh", NULL, "sw", NULL, NULL, NULL},
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
    if(target > BITMASK(25)) {
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
                 //dbg("imm", imm);
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


int main(int argc, char* argv[])
{
    FILE *fp;

    int c;
    int line_number = 0;
    char s1[10];
    char buffer[4] = {0, };
    

    fp = fopen(argv[1], "rb");

    while(!feof(fp)){
        fread(buffer,sizeof(buffer), 1, fp);
        if(feof(fp)) break;

        int rtr = ((unsigned char)buffer[0] << 24) | ((unsigned char)buffer[1] << 16) | ((unsigned char)buffer[2] << 8) | ((unsigned char)buffer[3] << 0);
        uint32_t number = rtr;
            mips_instruction_t* instruction = calloc(1, sizeof(mips_instruction_t));

          printf("inst %d: ", line_number);
            line_number += 1;


            if (mips_disassemble(instruction, number)) {
                printf("%08x ", number);
                if(!strcmp(instruction->name,"syscall"))
                {
                    printf("syscall");
                    printf("\n");
                    continue;
                }
                printf("%s %s", instruction->name, instruction->arguments);
            }
            else {
                printf("%08x ", number);
                printf("unknown instruction");
            }
            printf("\n");
            free(instruction);
        }
    

  fclose(fp);    


    return 0;
}




