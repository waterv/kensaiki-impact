%{
#include "main.h"
#include <cstring>

#define BufferLength 1024
#define CreateAtomWithCertainNumOfArgs(op, num)                                \
  do {                                                                         \
    auto atom = page->createAtom(op);                                          \
    atom->setArgsBy(args, num);                                                \
    atom->updateVal();                                                         \
    args.push(atom);                                                           \
  } while (0)
#define CreateUnaryAtom(op) CreateAtomWithCertainNumOfArgs(op, 1)
#define CreateBinaryAtom(op) CreateAtomWithCertainNumOfArgs(op, 2)
#define CreateTernaryAtom(op) CreateAtomWithCertainNumOfArgs(op, 3)
#define isFormalArg(atomPtr) (formalArgIndex.contains(atomPtr))
#define yyerror(str)                                                           \
  throw std::runtime_error { str }
#define YYERROR_VERBOSE 1

char TextBuffer[BufferLength];
int ReadOffset = 0;

Page *page;
std::stack<Atom *> args{};
std::stack<int> argNums{};
std::map<std::string, int> formalArgIndex{};
std::vector<std::string> formalArgs{};

extern std::map<std::string, std::pair<int, Func>> BuiltinFuncs;

extern "C" {
int yylex();
int yyparse();
int yywrap() { return 1; }
void FlushBuffer();

int ToDecimalInteger(char *string) {
  if (string[0] != '0')
    return 0;
  int base, i = 2, res = 0, digit, flag = 1;
  switch (string[1]) {
  case 'B':
  case 'b':
    base = 2;
    break;
  case 'O':
  case 'o':
    base = 8;
    break;
  case 'X':
  case 'x':
    base = 16;
    break;
  default:
    return 0;
  }
  if (string[2] == '+') {
    flag = 1;
    i += 1;
  } else if (string[2] == '-') {
    flag = -1;
    i += 1;
  }
  while (string[i] != '\0') {
    res *= base;
    if ('0' <= string[i] && string[i] <= '9')
      digit = string[i] - '0';
    else if ('A' <= string[i] && string[i] <= 'F')
      digit = string[i] - 'A' + 10;
    else
      digit = string[i] - 'a' + 10;
    if (digit >= base)
      return 0;
    res += digit;
    i += 1;
  }
  return res * flag;
}

int LexInput(char *buf, unsigned long *numRead, int maxRead) {
  int remain = strlen(TextBuffer) - ReadOffset;
  int numToRead = maxRead > remain ? remain : maxRead;
  memcpy(buf, TextBuffer + ReadOffset, numToRead);
  *numRead = numToRead;
  ReadOffset += numToRead;
  return 0;
}
}

Atom *parseLine(Page *pg, const std::string &str) {
  try {
    page = pg;
    memcpy(TextBuffer, str.c_str(), BufferLength);
    ReadOffset = 0;
    FlushBuffer();

    while (!args.empty())
      args.pop();
    while (!argNums.empty())
      argNums.pop();
    formalArgIndex.clear();
    formalArgs.clear();

    yyparse();

    auto atom = args.top();
    args.pop();
    return atom;
  } catch (std::runtime_error &e) {
    throw;
  }
}
%}

%union {
  int integer;
  double floating;
  char* string;
  struct {
    int type;
    double val;
  } literal;
}

%token PowOp DivOp ShlOp ShrOp GeOp LeOp NeOp AndOp OrOp AsgOp ToOp
%token <integer> DecimalIntegerLiteral BinaryIntegerLiteral
%token <integer> OctalIntegerLiteral HexadecimalIntegerLiteral
%token <floating> FloatingLiteral
%token <string> Identifier
%type <literal> Literal

%start Command

%%
Atom        : Identifier {
                if (isBuiltinFunction($1)) {
                  yyerror(fmt::format("'{}' is not a variable", $1));
                } else if (isFormalArg($1)) {
                  auto atom = page->createAtom(Reference);
                  atom->argIndex = formalArgIndex[$1];
                  args.push(atom);
                } else if (isIdentifierDefined(page, $1)) {
                  auto reference = page->symbols[$1];
                  if (!isVariable(reference))
                    yyerror(fmt::format("'{}' is not a variable", $1));
                  auto atom = page->createAtom(Reference);
                  reference->addReferree(atom);
                  atom->updateVal();
                  args.push(atom);
                } else {
                  yyerror(fmt::format("undefined identifier '{}'", $1));
                }
              }
            | Literal {
                auto atom = page->createAtom(Number);
                atom->type = (Type)$1.type;
                atom->val = $1.val;
                args.push(atom);
              }
            | '(' Expression ')'
            ;
Literal     : DecimalIntegerLiteral { $$ = {0, (double)$1}; }
            | BinaryIntegerLiteral { $$ = {1, (double)$1}; }
            | OctalIntegerLiteral { $$ = {2, (double)$1}; }
            | HexadecimalIntegerLiteral { $$ = {3, (double)$1}; }
            | FloatingLiteral { $$ = {0, $1}; }
            ;
Primary     : Atom | Call
            ;
Call        : Identifier '(' Arguments ')' {
                if (isBuiltinFunction($1)) {
                  if (argNums.top() != BuiltinFuncs[$1].first)
                    yyerror(fmt::format("wrong number of argument(s), needs {}, provides {}",
                                        BuiltinFuncs[$1].first, argNums.top()));
                  auto atom = page->createAtom(FuncCall);
                  atom->builtinFunc = BuiltinFuncs[$1].second;
                  atom->setArgsBy(args, argNums.top());
                  atom->updateVal();
                  args.push(atom);
                  argNums.pop();
                } else if (isIdentifierDefined(page, $1)) {
                  auto prototype = page->symbols[$1];
                  if (!isFunction(prototype))
                    yyerror(fmt::format("'{}' is not a function", $1));
                  if (argNums.top() != (int)prototype->formalArgs.size())
                    yyerror(fmt::format("wrong number of argument(s), needs {}, provides {}",
                                        (int)prototype->formalArgs.size(), argNums.top()));
                  auto atom = page->createAtom(FuncCall);
                  atom->setPrototype(prototype);
                  atom->setArgsBy(args, argNums.top());
                  atom->updateVal();
                  args.push(atom);
                  argNums.pop();
                } else {
                  yyerror(fmt::format("undefined identifier '{}'", $1));
                }
              }
            ;
Arguments   : Expression { argNums.push(1); }
            | Arguments ',' Expression { argNums.top() += 1; }
            ;
PowerExpr   : Primary
            | Primary PowOp UnaryExpr { CreateBinaryAtom(Power); }
            ;
UnaryExpr   : PowerExpr
            | '-' UnaryExpr { CreateUnaryAtom(Minus); }
            | '+' UnaryExpr { CreateUnaryAtom(Plus); }
            | '~' UnaryExpr { CreateUnaryAtom(Invert); }
            ;
TimesExpr   : UnaryExpr
            | TimesExpr '*' UnaryExpr { CreateBinaryAtom(Multiplication); }
            | TimesExpr DivOp UnaryExpr { CreateBinaryAtom(FloorDivision); }
            | TimesExpr '/' UnaryExpr { CreateBinaryAtom(Division); }
            | TimesExpr '%' UnaryExpr { CreateBinaryAtom(Modulo); }
            ;
AddExpr     : TimesExpr
            | AddExpr '+' TimesExpr { CreateBinaryAtom(Addition); }
            | AddExpr '-' TimesExpr { CreateBinaryAtom(Subtraction); }
            ;
ShiftExpr   : AddExpr
            | ShiftExpr ShlOp AddExpr { CreateBinaryAtom(LShifting); }
            | ShiftExpr ShrOp AddExpr { CreateBinaryAtom(RShifting); }
            ;
AndExpr     : ShiftExpr
            | AndExpr '&' ShiftExpr { CreateBinaryAtom(BitAnd); }
            ;
XorExpr     : AndExpr
            | XorExpr '^' AndExpr { CreateBinaryAtom(BitXor); }
            ;
OrExpr      : XorExpr
            | OrExpr '|' XorExpr { CreateBinaryAtom(BitOr); }
            ;
Comparison  : OrExpr
            | OrExpr '<' OrExpr { CreateBinaryAtom(Less); }
            | OrExpr '>' OrExpr { CreateBinaryAtom(Greater); }
            | OrExpr '=' OrExpr { CreateBinaryAtom(Equal); }
            | OrExpr GeOp OrExpr { CreateBinaryAtom(GEqual); }
            | OrExpr LeOp OrExpr { CreateBinaryAtom(LEqual); }
            | OrExpr NeOp OrExpr { CreateBinaryAtom(NEqual); }
            ;
NotTest     : Comparison
            | '!' NotTest { CreateBinaryAtom(Not); }
            ;
AndTest     : NotTest
            | AndTest AndOp NotTest { CreateBinaryAtom(And); }
            ;
OrTest      : AndTest
            | OrTest OrOp AndTest { CreateBinaryAtom(Or); }
            ;
Definition  : Identifier AsgOp FormalArg ToOp Expression {
                if (isIdentifierDefined(page, $1))
                  yyerror(fmt::format("redefinition of identifier '{}'", $1));
                if (isBuiltinFunction($1))
                  yyerror(fmt::format("redefinition of keyword '{}'", $1));
                args.top()->symbol = $1;
                args.top()->formalArgs = formalArgs;
              }
            ;
FormalArg   : Identifier {
                if (isFormalArg($1))
                  yyerror(fmt::format("arguments with duplicated name '{}'", $1));
                formalArgs.push_back($1);
                formalArgIndex[$1] = (int)formalArgIndex.size();
              }
            | FormalArg ',' Identifier {
                if (isFormalArg($3))
                  yyerror(fmt::format("arguments with duplicated name '{}'", $3));
                formalArgs.push_back($3);
                formalArgIndex[$3] = (int)formalArgIndex.size();
              }
            ;
Assignment  : Identifier AsgOp Expression {
                if (isIdentifierDefined(page, $1))
                  yyerror(fmt::format("redefinition of identifier '{}'", $1));
                if (isBuiltinFunction($1))
                  yyerror(fmt::format("redefinition of keyword '{}'", $1));
                args.top()->symbol = $1;
              }
            ;
Expression  : OrTest
            | OrTest '?' OrTest ':' Expression { CreateTernaryAtom(If); }
            ;
Command     : Definition
            | Assignment
            | Expression
            ;
%%
