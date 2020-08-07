%code requires {
#include "assembler_internal.h"
extern void yyerror(const char *err);
extern int yylex();
}

%union {
    int intval;
    uint8_t byteval;
    byte_array_t bytearray = { 0, 0 };
    char *strval;
    opcode_entry_t *opcode;
}

%token <opcode> INSTRUCTION
%token <strval> SYMBOL QUOTED_STRING 
%token <intval> WORD_LITERAL INTEGER_LITERAL REGISTER_ARGUMENT INCREMENT
%token <byteval> BYTE_LITERAL
%token <bytearray> BYTE_SEQUENCE
%token DEFBYTE_DIRECTIVE DEFWORD_DIRECTIVE INCLUDE_DIRECTIVE DATA_DIRECTIVE COMMENT

%type <intval> register_list indexed_register
%type <bytearray> byte_sequence
%%

line : line comment
        | instruction_line
        | include_def     
        | byte_def
        | word_def
        | label_def
        | data_def
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
        ;

byte_sequence : BYTE_LITERAL { $$ = add_to_byte_array($$, $1); }
        | byte_sequence BYTE_LITERAL { $$ = add_to_byte_array($1, $2); }
        ;

instruction_line : INSTRUCTION SYMBOL { handle_instruction(assembler_data, $1, $2, 0); }
        | INSTRUCTION BYTE_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION WORD_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION INTEGER_LITERAL { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION register_list { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION indexed_register { handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION { handle_instruction(assembler_data, $1, 0, 0); }
        ;

register_list : REGISTER_ARGUMENT ',' REGISTER_ARGUMENT ',' REGISTER_ARGUMENT ',' REGISTER_ARGUMENT { $$ = (TO_DESTINATION($1) | TO_SOURCE_0($3)) | (TO_SOURCE_1($5) | TO_SOURCE_2($7)); }
        | REGISTER_ARGUMENT ',' REGISTER_ARGUMENT ',' REGISTER_ARGUMENT { $$ = (TO_DESTINATION($1) | TO_SOURCE_0($3)) | (TO_SOURCE_1($5)); }
        | REGISTER_ARGUMENT ',' REGISTER_ARGUMENT { $$ = (TO_DESTINATION($1) | TO_SOURCE_0($3)); }
        | REGISTER_ARGUMENT { $$ = TO_DESTINATION($1); }
        ;

indexed_register : '[' REGISTER_ARGUMENT ']' { $$ = TO_SOURCE_2($2); }
        | '[' REGISTER_ARGUMENT INCREMENT ']' { $$ = TO_SOURCE_2($2) | (($3 < 0) ? (TO_SOURCE_1(-$3)) : (TO_SOURCE_0($3))); }
        ;

comment : COMMENT 
        ;

%%
