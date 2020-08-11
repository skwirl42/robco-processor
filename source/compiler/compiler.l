%{
#include <stdio.h>
#include "compiler_internal.hpp"
#include "compiler_yacc.hpp"
%}

%option never-interactive
%option noyywrap

SYMBOL                  [_a-zA-Z][_a-zA-Z0-9]*
HEX_LITERAL             0x[a-fA-F0-9]+
INTEGER_LITERAL         [+\-]?[0-9]+
BINARY_LITERAL          0b[01]+
ESCAPE_SEQUENCE         \\["'\\abrnt0]
WHITESPACE              [ \t]+

%%

{HEX_LITERAL} {
    sscanf(yytext, "0x%x", &yylval.intval);
    return HEX_LITERAL;
}

{INTEGER_LITERAL} {
    sscanf(yytext, "%d", &yylval.intval);
    return INTEGER_LITERAL;
}

{BINARY_LITERAL} {
    return BINARY_LITERAL;
}

{SYMBOL} {
    yylval.stringval = new std::string(yytext);
    return SYMBOL;
}

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
