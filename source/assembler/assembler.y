%code requires {
#include "assembler_internal.h"
extern void yyerror(const char *err);
extern int yylex();
}

%union {
    int intval;
    uint8_t *bytearray;
    char *strval;
    opcode_entry_t *opcode;
}

%token <opcode> INSTRUCTION
%token <strval> SYMBOL QUOTED_STRING 
%token <intval> BYTE_LITERAL WORD_LITERAL INTEGER_LITERAL
%token <bytearray> BYTE_SEQUENCE
%token DEFBYTE_DIRECTIVE DEFWORD_DIRECTIVE INCLUDE_DIRECTIVE DATA_DIRECTIVE COMMENT
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

include_def : INCLUDE_DIRECTIVE QUOTED_STRING { auto include_result = handle_include(assembler_data, $2); free($2); }
        ;

byte_def : DEFBYTE_DIRECTIVE SYMBOL BYTE_LITERAL { auto define_result = handle_symbol_def(assembler_data, $2, $3, SYMBOL_BYTE); free($2); }
        | DEFBYTE_DIRECTIVE SYMBOL INTEGER_LITERAL
        {
                if ($3 > 255 || $3 < -128)
                {
                        fprintf(stderr, "Integer out of range for byte size %d\n", $3);
                }
                else
                {
                        auto define_result = handle_symbol_def(assembler_data, $2, $3, SYMBOL_BYTE);
                }
                free($2);
        }
        ;

word_def : DEFWORD_DIRECTIVE SYMBOL WORD_LITERAL { auto define_result = handle_symbol_def(assembler_data, $2, $3, SYMBOL_WORD); free($2); }
        | DEFWORD_DIRECTIVE SYMBOL INTEGER_LITERAL
        {
                if ($3 > 65535 || $3 < -32768)
                {
                        fprintf(stderr, "Integer out of range for word size %d\n", $3);
                }
                else
                {
                        auto define_result = handle_symbol_def(assembler_data, $2, $3, SYMBOL_WORD);
                }
                free($2);
        }
        ;

label_def : SYMBOL ':' { auto define_result = handle_symbol_def(assembler_data, $2, 0, SYMBOL_ADDRESS_INST); free($2); }
        ;

data_def : DATA_DIRECTIVE SYMBOL BYTE_SEQUENCE
        | DATA_DIRECTIVE BYTE_SEQUENCE
        ;

instruction_line : INSTRUCTION SYMBOL { auto opcode_result = handle_instruction(assembler_data, $1, $2, 0); }
        | INSTRUCTION BYTE_LITERAL { auto opcode_result = handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION WORD_LITERAL { auto opcode_result = handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION INTEGER_LITERAL { auto opcode_result = handle_instruction(assembler_data, $1, 0, $2); }
        | INSTRUCTION { auto opcode_result = handle_instruction(assembler_data, $1, 0, 0); }
        ;

comment : COMMENT 
        ;

%%
