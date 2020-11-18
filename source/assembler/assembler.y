%code requires {
#include "assembler_internal.h"
extern void yyerror(const char *err);
extern int yylex();
}

%union {
    int intval;
    uint8_t byteval;
    byte_array_t bytearray;
    char *strval;
    opcode_entry_t *opcode;
    register_index_t index_register;
}

%token <opcode> INSTRUCTION
%token <strval> SYMBOL QUOTED_STRING 
%token <intval> WORD_LITERAL INTEGER_LITERAL CHAR_LITERAL INCREMENT
%token <byteval> BYTE_LITERAL
%token <bytearray> BYTE_SEQUENCE
%token <index_register> REGISTER_ARGUMENT
%token DEFBYTE_DIRECTIVE DEFWORD_DIRECTIVE INCLUDE_DIRECTIVE DATA_DIRECTIVE RESERVE_DIRECTIVE ORG_DIRECTIVE COMMENT

%type <index_register> indexed_register
%type <bytearray> byte_sequence
%%

line : line comment
        | instruction_line
        | include_def     
        | byte_def
        | word_def
        | label_def
        | data_def
        | reserve_def
        | org_def
        | comment
        ;

include_def : INCLUDE_DIRECTIVE QUOTED_STRING { add_file_to_process(assembler_data, $2); free($2); }
        ;

byte_def : DEFBYTE_DIRECTIVE SYMBOL BYTE_LITERAL { handle_symbol_def(assembler_data, $2, $3, SYMBOL_BYTE); free($2); }
        | DEFBYTE_DIRECTIVE SYMBOL INTEGER_LITERAL
        {
                if ($3 > 255 || $3 < -128)
                {
                        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Integer out of range for byte size %d", $3);
                        add_error(assembler_data, temp_buffer, ASSEMBLER_VALUE_OOB);
                }
                else
                {
                        handle_symbol_def(assembler_data, $2, $3, SYMBOL_BYTE);
                }
                free($2);
        }
        ;

word_def : DEFWORD_DIRECTIVE SYMBOL WORD_LITERAL { handle_symbol_def(assembler_data, $2, $3, SYMBOL_WORD); free($2); }
        | DEFWORD_DIRECTIVE SYMBOL INTEGER_LITERAL
        {
                if ($3 > 65535 || $3 < -32768)
                {
                        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Integer out of range for word size %d", $3);
                        add_error(assembler_data, temp_buffer, ASSEMBLER_VALUE_OOB);
                }
                else
                {
                        handle_symbol_def(assembler_data, $2, $3, SYMBOL_WORD);
                }
                free($2);
        }
        ;

label_def : SYMBOL ':' { handle_symbol_def(assembler_data, $1, 0, SYMBOL_ADDRESS_INST); free($1); }
        ;

data_def : DATA_DIRECTIVE SYMBOL byte_sequence { add_data(assembler_data, $2, $3); free($2); }
        | DATA_DIRECTIVE byte_sequence  { add_data(assembler_data, nullptr, $2); }
        | DATA_DIRECTIVE SYMBOL QUOTED_STRING { add_string_to_data(assembler_data, $2, $3); free($2); free($3); }
        | DATA_DIRECTIVE QUOTED_STRING { add_string_to_data(assembler_data, nullptr, $2); free($2); }
        ;

reserve_def : RESERVE_DIRECTIVE SYMBOL INTEGER_LITERAL { reserve_data(assembler_data, $2, $3); free($2); }
        ;

org_def : ORG_DIRECTIVE WORD_LITERAL { handle_org_directive(assembler_data, $2); }
        ;

byte_sequence : BYTE_LITERAL { $$ = add_to_byte_array($$, $1); }
        | byte_sequence BYTE_LITERAL { $$ = add_to_byte_array($1, $2); }
        ;

instruction_line : INSTRUCTION SYMBOL { handle_instruction(assembler_data, $1, $2, 0); }
        | INSTRUCTION BYTE_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION WORD_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION INTEGER_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION CHAR_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION indexed_register { handle_indexed_instruction(assembler_data, $1, $2); }
        | INSTRUCTION { handle_instruction(assembler_data, $1, 0, 0); }
        ;

indexed_register : '[' REGISTER_ARGUMENT ']' { $$ = $2; $$.increment_amount = 0; }
        | '[' REGISTER_ARGUMENT INCREMENT ']' { $$ = $2; $$.increment_amount = $3; }
        | '[' INCREMENT REGISTER_ARGUMENT ']' { $$ = $3; $$.increment_amount = $2; $$.is_pre_increment = 1; }
        ;

comment : COMMENT 
        ;

%%
