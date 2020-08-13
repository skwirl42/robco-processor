%{
#include <stdio.h>
#include "compiler_internal.hpp"
#include "compiler_yacc.hpp"
%}

%option never-interactive
%option noyywrap

SYMBOL                  [_a-zA-Z][_a-zA-Z0-9]*
HEX_LITERAL             0x[a-fA-F0-9]+
DECIMAL_LITERAL         [+\-]?[0-9]+
OCTAL_LITERAL           0[0-7]+
BINARY_LITERAL          0b[01]+
ESCAPE_SEQUENCE         \\["'\\abrnt0]
WHITESPACE              [ \t]+

%%

{OCTAL_LITERAL} {
    sscanf(yytext, "%o", &yylval.intval);
    return INTEGER_LITERAL;
}

{HEX_LITERAL} {
    sscanf(yytext, "0x%x", &yylval.intval);
    return INTEGER_LITERAL;
}

{DECIMAL_LITERAL} {
    sscanf(yytext, "%d", &yylval.intval);
    return INTEGER_LITERAL;
}

{BINARY_LITERAL} {
    yylval.intval = 0;
    for (int i = 2; i < yyleng; i++)
    {
        yylval.intval <<= 1;
        if (yytext[i] == '1')
        {
            yylval.intval |= 1;
        }
    }
    return INTEGER_LITERAL;
}

{SYMBOL} {
    yylval.stringval = new std::string(yytext);
    return SYMBOL;
}

{WHITESPACE} ;

. return yytext[0];

%%

extern int yylex();
extern int yyparse();

void yyerror(const char *err)
{
    //snprintf(temp_error_buffer, ERROR_BUFFER_SIZE, "Flex error (%s) on line %d of file %s", err, compiler_data->lineNumber, compiler_data->current_filename);
    compiler_data->add_error(err, CompilerError::SyntaxError);
}

void parse_line(const char *lineBuffer)
{
    YY_BUFFER_STATE buffer = yy_scan_string( lineBuffer );
    yyparse();
    yy_delete_buffer( buffer );
}
