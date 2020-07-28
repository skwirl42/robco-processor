%{
#include <stdio.h>
#include "assembler_yacc.hpp"
%}

%option noyywrap

SYMBOL                  [a-zA-Z][a-zA-Z0-9_]+
BYTE_LITERAL            0x[a-zA-Z0-9]{2}
WORD_LITERAL            0x[a-zA-Z0-9]{4}
INCLUDE_DIRECTIVE       \.include
DEFBYTE_DIRECTIVE       \.defbyte
DEFWORD_DIRECTIVE       \.defword
DATA_DIRECTIVE          \.data
ESCAPE_SEQUENCE         \\['\\abrnt0]
WHITESPACE              \s+
QUOTE                   '
DOUBLEQUOTE             "

%%

{BYTE_LITERAL} {
    sscanf(yytext, "0x%x", &yylval.intval);
    return BYTE_LITERAL;
}

{WORD_LITERAL} {
    sscanf(yytext, "0x%x", &yylval.intval);
    return WORD_LITERAL;
}

{DEFBYTE_DIRECTIVE} {
    return DEFBYTE_DIRECTIVE;
}

{DEFWORD_DIRECTIVE} {
    return DEFWORD_DIRECTIVE;
}

{SYMBOL} {
    yylval.strval = strdup(yytext);
    return SYMBOL;
}

{WHITESPACE} ;

. return yytext[0];

%%

extern int yylex();

void yyerror(const char *err)
{
    fprintf(stderr, "Flex error (%s)\n", err);
}

void process_line(const char *line)
{
    YY_BUFFER_STATE buffer = yy_scan_string(line);
    yylex();
    yy_delete_buffer(buffer);
}