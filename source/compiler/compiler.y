%code requires {
#include "compiler_internal.hpp"
extern void yyerror(const char *err);
extern int yylex();
}

%union {
    int intval;
    std::string *stringval;
}

%token <stringval> SYMBOL
%token <intval> INTEGER_LITERAL BINARY_LITERAL

%type <intval> numeric_literal
%%

root : expression
    ;

expression : numeric_literal
    ;

numeric_literal :  INTEGER_LITERAL      { $$ = $1; }
    ;

%%