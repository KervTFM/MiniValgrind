%{
   #include <string>
   #define YYSTYPE std::string
   #include "parser.hpp"
   void yyerror(char *s);
%}

%option yylineno
%option noyywrap

%x STR

%%

[/][/].*\n                      ; // comment
if                              return IF;
else                            return ELSE;
while                           return WHILE;
return                          return RETURN;
int                             return INT;
ptr                             return PTR;
arr                             return ARR;
==                              return EQ;
[<]=                            return LE;
>=                              return GE;
!=                              return NE;
&&                              return AND;
\|\|                            return OR;
#begin                          return P_BEGIN;
#end                            return P_END;

[0-9]+                          {
                                    yylval = yytext;
                                    return NUM;
                                }

[a-zA-Z_][a-zA-Z0-9_]*          {
                                    yylval = yytext;
                                    return ID;
                                }

[ \t\r\n]                       ; // whitespace
[-{};()=<>+*%&/!,\[\]]          { return *yytext; }
.                               yyerror("Invalid character");

%%
