%{
extern void yyerror(const char *err);
extern int yylex();
%}

%union {
    int intval;
    const char *strval;
}

%token SYMBOL DEFBYTE_DIRECTIVE BYTE_LITERAL DEFWORD_DIRECTIVE WORD_LITERAL QUOTED_STRING
%%

byte_def : DEFBYTE_DIRECTIVE SYMBOL BYTE_LITERAL
        ;

word_def : DEFWORD_DIRECTIVE SYMBOL WORD_LITERAL
        ;

%%