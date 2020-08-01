%{
#include <stdio.h>
#include "assembler_yacc.hpp"
#include "assembler_internal.h"
%}

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

%%

{BYTE_LITERAL} {
    sscanf(yytext, "0x%x", &yylval.intval);
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

{INSTRUCTION} {
    yylval.opcode = get_opcode_entry(yytext);
    if (yylval.opcode)
    {
        // fprintf(stdout, "Got opcode %s\n", yytext);
        return INSTRUCTION;
    }
    else
    {
        // fprintf(stderr, "Thought %s might be an instruction, but it ain't\n", yytext);
        // This may not be exactly the behaviour we want. It'll mean typos will be seen as symbols,
        // although that does mean it'll never match one of our parser rules, which also works
        yylval.strval = strdup(yytext);
        return SYMBOL;
    }
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
    yylval.strval[yyleng-2] = 0;
    return QUOTED_STRING;
}

{WHITESPACE} ;

. return yytext[0];

%%

extern int yylex();

void yyerror(const char *err)
{
    fprintf(stderr, "Flex error (%s) on line %d of file %s\n", err, assembler_data->lineNumber, assembler_data->current_filename);
}

void parse_line(const char *lineBuffer)
{
    YY_BUFFER_STATE buffer = yy_scan_string( lineBuffer );
    yyparse();
    yy_delete_buffer( buffer );
}
