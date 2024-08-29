#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "code.h"
#include "utils.h"


typedef struct cmd_element {
    char *cmd;
    opcode op;
}cmd_element;

static cmd_element cmd_lookup_table[] = {
        {"mov", MOV_OP},
        {"cmp", CMP_OP},
        {"add", ADD_OP},
        {"sub", SUB_OP},
        {"lea", LEA_OP},
        {"clr", CLR_OP},
        {"not", NOT_OP},
        {"inc", INC_OP},
        {"dec", DEC_OP},
        {"jmp", JMP_OP},
        {"bne", BNE_OP},
        {"jsr", JSR_OP},
        {"red", RED_OP},
        {"prn", PRN_OP},
        {"rts", RTS_OP},
        {"stop", STOP_OP},
        {NULL, NONE_OP}
};


opcode get_opcode_by_name(const char *name){
    cmd_element *e;

    for(e = cmd_lookup_table; e != NULL; e++){
        if(strcmp(name, e->cmd) == 0) return e->op;
    }
    return NONE_OP;
}


reg get_register_by_name(const char *name){
    int digit;
    if(name[1] && name[0] == 'r' && !name[2]){
        digit = name[1] - '0'; //convert digit to the difference between 0 in ASCII;
        if(digit >= 0 && digit <= 7) return digit;
    }
    return NONE_REG;
}



bool extract_operands(line_info line, int index, int *operand_count, char **operands){
    char *temp_operand;
    int i;

    *operand_count = 0;

    SKIP_WHITE_SPACES(line.content, index);
    if(line.content[index] = ','){
        printf_line_error(line, "unexpected comma after opcode.");
        return false;
    }

    while(line.content[index] != '\n' && line.content[index] != EOF){
        if(*operand_count == 2){
            printf_line_error(line, "more than 2 arguements given to opcode.");
            return false;
        }


        for (i = 0;
		      line.content[index] && !isspace(line.content[index]) &&
               line.content[index] != ',' ; index++, i++) {
			temp_operand[i] = line.content[index];
		}
        temp_operand[i] = '\0';
        
        strcpy(operands[*operand_count], temp_operand);
        
        SKIP_WHITE_SPACES(line.content, index);
        if(!line.content[index] || line.content[index] == '\n' || line.content[index] == EOF){
            break; //end of line. ran successfuly
        }
        if(line.content[index] = ','){
            index++;
            SKIP_WHITE_SPACES(line.content, index);

            if(line.content[index] = ","){
                printf_line_error(line, "multiple commas in a row.");
                return false;
            }
            if(!line.content[index] || line.content[index] == '\n' || line.content[index] == EOF){
                printf_line_error(line, "line could not end with a comma.");
                return false;
            }

            (*operand_count)++;
            continue;
        }
        else{
            printf_line_error(line, "2 arguments without comma between them.");
            return false;
        }
    }
    return true;   
}

addressing_type get_addressing_type(char *operand) {
    if (operand[0] == '#' && is_int(operand + 1)) return IMMEDIATE_ADDR;

    if (is_valid_label_name(operand)) return DIRECT_ADDR;

    if (operand[0] == '*' && get_register_by_name(operand + 1) != NONE_REG) return DIRECT_REGISTER_ADDR;

	if (get_register_by_name(operand + 1) != NONE_REG) return INDIRECT_REGISTER_ADDR;

	return NONE_ADDR;
}

code_word *get_code_word(line_info line, opcode curr_opcode, int op_count, char **operands){
    code_word *codeword;

    addressing_type src_addressing = op_count >= 1 ? get_addressing_type(operands[0]) : NONE_ADDR;
    addressing_type dest_addressing = op_count == 2 ? get_addressing_type(operands[0]) : NONE_ADDR;

    if(!validate_opcode_with_operands(line, curr_opcode, src_addressing, dest_addressing, op_count)){
        return NULL;
    }

    codeword = (code_word *)malloc_with_check(sizeof(code_word));

    codeword->opcode = curr_opcode;
    codeword->ARE = (1 << 2); // A = 1, R = 0, E = 0 for each codeword.

    codeword->dest_addressing = codeword->src_addressing = NONE_ADDR;
    if (curr_opcode >= MOV_OP && curr_opcode <= LEA_OP){
        codeword->src_addressing = (1 << src_addressing); //for example: DIRECT_ADDR is 1, and 0001 << 1 will give us 0010.
        codeword->dest_addressing = (1 << dest_addressing);
    }
    else if(curr_opcode >= CLR_OP && curr_opcode <= PRN_OP){
        codeword->dest_addressing = (1 << src_addressing);
    }

    return codeword;

}

bool validate_opcode_with_operands(line_info line, opcode curr_opcode, addressing_type src_addressing, addressing_type dest_addressing, int op_count){
    bool legal_src_addr_types[4] = {false}, legal_dest_addr_types[4] = {false};
    
    get_available_addressing_types(curr_opcode, legal_src_addr_types, legal_dest_addr_types);

    if(curr_opcode >= MOV_OP && curr_opcode <= LEA_OP){
        //2 operands required.
        if(op_count != 2){
            printf_line_error(line, "illegal number of operands given to opcode. required: 2. found: %d", op_count);
            return false;
        }
        if(src_addressing != NONE_ADDR && !legal_src_addr_types[src_addressing]){
            printf_line_error(line, "illegal addressing type for first operand");
            return false;
        }
        if(dest_addressing != NONE_ADDR && !legal_dest_addr_types[dest_addressing]){
            printf_line_error(line, "illegal addressing type for second operand.");
            return false;
        }
        return true;
    }
    if(curr_opcode >= CLR_OP && curr_opcode <= PRN_OP){
        //1 operand required.
        if(op_count != 1){
            printf_line_error(line, "illegal number of operands given to opcode. required: 1. found: %d", op_count);
            return false;
        }
        if(src_addressing != NONE_ADDR && !legal_src_addr_types[src_addressing]){
            printf_line_error(line, "illegal addressing type for operand");
            return false;
        }
        return true;
    }
    if (curr_opcode <= STOP_OP && curr_opcode >= RTS_OP){
        //0 operands required.
        if(op_count != 0){
            printf_line_error(line, "illegal number of operands given to opcode. required: 0. found: %d", op_count);
            return false;
        }
        return true;
    }
}

void get_available_addressing_types(opcode curr_opcode, bool *legal_src_addr_types, bool *legal_dest_addr_types){

    //handle src first.
    if((curr_opcode >= MOV_OP && curr_opcode <= DEC_OP) || curr_opcode == RED_OP || curr_opcode == PRN_OP){
        legal_src_addr_types[DIRECT_ADDR] = true;
        legal_src_addr_types[INDIRECT_REGISTER_ADDR] = true;
        legal_src_addr_types[DIRECT_REGISTER_ADDR] = true;

        if(curr_opcode == CMP_OP || curr_opcode == PRN_OP){
            legal_src_addr_types[IMMEDIATE_ADDR] = true;
        }
    }
    else if(curr_opcode == JMP_OP || curr_opcode == BNE_OP || curr_opcode == JSR_OP){
        legal_src_addr_types[DIRECT_ADDR] = true;
        legal_src_addr_types[INDIRECT_REGISTER_ADDR] = true;
    }

    //handle dest at last.
    if(curr_opcode >= MOV_OP && curr_opcode <= SUB_OP){
        legal_dest_addr_types[IMMEDIATE_ADDR] = true;
        legal_dest_addr_types[DIRECT_ADDR] = true;
        legal_dest_addr_types[INDIRECT_REGISTER_ADDR] = true;
        legal_dest_addr_types[DIRECT_REGISTER_ADDR] = true;
    }
    else if(curr_opcode == LEA_OP){
        legal_dest_addr_types[DIRECT_ADDR] = true;
    }
}


data_word *build_data_word(addressing_type addressing, long data, bool is_extern_symbol) {
	unsigned long ARE = 4; /* 4 = 2^2 = 1 << 2  - IMMEDIATE_ADDR - only A is 1.*/
	data_word *dataword = (data_word *) malloc_with_check(sizeof(data_word));

	if (addressing == DIRECT_ADDR) {
		ARE = is_extern_symbol ? 1 : 2;
	}
	dataword->ARE = ARE; /* Set ARE field value */
	dataword->data = data; 

	return dataword;
}

register_word *build_register_word(reg src_register, reg dest_register){
    unsigned long ARE = 4; /* 4 = 2^2 = 1 << 2  - DIRECT_REGISTER_ADDR or INDIRECT_REGISTER_ADDR - only A is 1.*/
    register_word *registerword = (register_word *) malloc_with_check(sizeof(register_word));
    
    registerword->src_register = 0;
    registerword->dest_register = 0;

    if(src_register != NONE_REG){
        registerword->src_register = src_register;
    }
    if(dest_register != NONE_REG){
        registerword->dest_register = dest_register;
    }
    registerword->ARE = ARE;

    return registerword;
}