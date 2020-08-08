%{
#include <stdio.h>
#include "assembler_yacc.hpp"
#include "assembler_internal.h"
%}

%option never-interactive
%option noyywrap

SYMBOL                  [a-zA-Z][a-zA-Z0-9_]*
BYTE_LITERAL            0x[a-fA-F0-9]{2}
WORD_LITERAL            0x[a-fA-F0-9]{4}
INTEGER_LITERAL         [+\-]?[0-9]+
INCLUDE_DIRECTIVE       \.include
DEFBYTE_DIRECTIVE       \.defbyte
DEFWORD_DIRECTIVE       \.defword
DATA_DIRECTIVE          \.data
ESCAPE_SEQUENCE         \\['\\abrnt0]
WHITESPACE              [ \t]+
INSTRUCTION             [a-z]{1,7}
REGISTER_ARGUMENT       [a-z]{1,2}
INCREMENT               \+{1,2}
DECREMENT               \-{1,2}

%%

{BYTE_LITERAL} {
    int val;
    sscanf(yytext, "0x%02x", &val);
    yylval.byteval = val;
    return BYTE_LITERAL;
}

{WORD_LITERAL} {
    sscanf(yytext, "0x%x", &yylval.intval);
    return WORD_LITERAL;
}

{INTEGER_LITERAL} {
    sscanf(yytext, "%d", &yylval.intval);
    return INTEGER_LITERAL;
}

{INCLUDE_DIRECTIVE} {
    return INCLUDE_DIRECTIVE;
}

{DEFBYTE_DIRECTIVE} {
    return DEFBYTE_DIRECTIVE;
}

{DEFWORD_DIRECTIVE} {
    return DEFWORD_DIRECTIVE;
}

{DATA_DIRECTIVE} {
    return DATA_DIRECTIVE;
}

{INCREMENT} {
    yylval.intval = yyleng;
    return INCREMENT;
}

{DECREMENT} {
    yylval.intval = -yyleng;
    return INCREMENT;
}

{REGISTER_ARGUMENT} {
    register_argument_t *register_entry = get_register(yytext);
    if (register_entry)
    {
        yylval.intval = register_entry->code;
        return REGISTER_ARGUMENT;
    }

    yylval.opcode = get_opcode_entry(yytext);
    if (yylval.opcode)
    {
        return INSTRUCTION;
    }

    yylval.strval = strdup(yytext);
    return SYMBOL;
}

{INSTRUCTION} {
    yylval.opcode = get_opcode_entry(yytext);
    if (yylval.opcode)
    {
        return INSTRUCTION;
    }

    yylval.strval = strdup(yytext);
    return SYMBOL;
}

{SYMBOL} {
    yylval.strval = strdup(yytext);
    return SYMBOL;
}

\;.+ {
    return COMMENT;
}

\"[^\n\"]+\" {
    //fprintf(stdout, "Got quoted string: %s\n", yytext);
    yylval.strval = strdup(yytext+1);
    if (yylval.strval[yyleng-3] == 'n' && yylval.strval[yyleng-4] == '\\')
    {
        yylval.strval[yyleng-4] = 0x0A;
        yylval.strval[yyleng-3] = 0;
    }
    else
    {
        yylval.strval[yyleng-2] = 0;
    }
    
    return QUOTED_STRING;
}

{WHITESPACE} ;

. return yytext[0];

%%

extern int yylex();

char temp_error_buffer[ERROR_BUFFER_SIZE+1];

void yyerror(const char *err)
{
    snprintf(temp_error_buffer, ERROR_BUFFER_SIZE, "Flex error (%s) on line %d of file %s", err, assembler_data->lineNumber, assembler_data->current_filename);
    add_error(assembler_data, temp_error_buffer, ASSEMBLER_SYNTAX_ERROR);
}

void parse_line(const char *lineBuffer)
{
    YY_BUFFER_STATE buffer = yy_scan_string( lineBuffer );
    yyparse();
    yy_delete_buffer( buffer );
}
