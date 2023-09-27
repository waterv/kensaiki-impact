%{
#include "parser.h"
#include <stdio.h>

#undef YY_INPUT
#define YY_INPUT(b, r, s) LexInput((b), &(r), (s))

void FlushBuffer() { YY_FLUSH_BUFFER; }
extern int ToDecimalInteger(char *string);
extern int LexInput(char *buf, unsigned long *numRead, int maxRead);
%}

%%
[0-9]+                        sscanf(yytext, "%d", &yylval.integer);     return DecimalIntegerLiteral;
0[Bb][0-1]+                   yylval.integer = ToDecimalInteger(yytext); return BinaryIntegerLiteral;
0[Oo][0-7]+                   yylval.integer = ToDecimalInteger(yytext); return OctalIntegerLiteral;
0[Xx][A-Fa-f0-9]+             yylval.integer = ToDecimalInteger(yytext); return HexadecimalIntegerLiteral;
[0-9]*\.[0-9]+                sscanf(yytext, "%lf", &yylval.floating);   return FloatingLiteral;
[0-9]+[eE][+-]?[0-9]+         sscanf(yytext, "%lf", &yylval.floating);   return FloatingLiteral;
[0-9]*\.[0-9]+[eE][+-]?[0-9]+ sscanf(yytext, "%lf", &yylval.floating);   return FloatingLiteral;
[_A-Za-z][_A-Za-z0-9]*        yylval.string = strdup(yytext);            return Identifier;
"**" return PowOp;
"//" return DivOp;
"<<" return ShlOp;
">>" return ShrOp;
">=" return GeOp;
"<=" return LeOp;
"!=" return NeOp;
"&&" return AndOp;
"||" return OrOp;
":=" return AsgOp;
"=>" return ToOp;
[ \n\t]+ ;
. return (int)yytext[0];
%%
