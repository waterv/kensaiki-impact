#ifndef __MAIN_H__
#define __MAIN_H__

#include <fmt/core.h>
#include <fmt/format.h>

#include <fstream>
#include <map>
#include <numeric>
#include <queue>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#define isBuiltinFunction(id) (BuiltinFuncs.contains(id))
#define isIdentifierDefined(page, id) ((page)->symbols.contains(id))
#define isFunction(atomPtr) (!(atomPtr)->formalArgs.empty())
#define isVariable(atomPtr) (!isFunction(atomPtr) && (atomPtr)->symbol[0])
#define isNumberLiteral(atomPtr) ((atomPtr)->op == Number)
#define isReference(atomPtr) ((atomPtr)->op == Reference)
#define isVariableReference(atomPtr)                                           \
  (isReference(atomPtr) && !(atomPtr)->args.empty())
#define isFunctionArgument(atomPtr)                                            \
  (isReference(atomPtr) && (atomPtr)->args.empty())
#define isUnaryOperator(atomPtr) (OpArgNum[(atomPtr)->op] == 1)
#define isBinaryOperator(atomPtr) (OpArgNum[(atomPtr)->op] == 2)
#define isBaseInputDisabled(atomPtr)                                           \
  ((atomPtr)->type == Decimal || (atomPtr)->type == Boolean)

enum Op {
  Number,
  Reference,
  FuncCall,
  Power,
  Minus,
  Plus,
  Invert,
  Multiplication,
  FloorDivision,
  Division,
  Modulo,
  Addition,
  Subtraction,
  LShifting,
  RShifting,
  BitAnd,
  BitXor,
  BitOr,
  Less,
  Greater,
  Equal,
  GEqual,
  LEqual,
  NEqual,
  Not,
  And,
  Or,
  If,
  OpCount
};
enum Func {
  NoBuiltin,
  Abs,
  Max,
  Min,
  Sqrt,
  Cbrt,
  Sin,
  Cos,
  Tan,
  ArcSin,
  ArcCos,
  ArcTan,
  Sinh,
  Cosh,
  Tanh,
  ArcSinh,
  ArcCosh,
  ArcTanh,
  Erf,
  Erfc,
  Exp,
  Ln,
  Lg,
  Lb,
  Log,
  Floor,
  Ceil,
  Round,
  Trunc,
  Gamma,
  ToDec,
  ToBin,
  ToOct,
  ToHex,
  Is,
  IsInf,
  IsNan,
  Random,
  Gcd,
  Lcm,
  Rad,
  Deg,
};
enum Type { Decimal, Binary, Octal, Hexadecimal, Boolean, TypeCount };

struct Atom;
struct Line;
struct Page;
using Token = std::pair<std::string, Atom *>;

extern "C" {
int ToDecimalInteger(char *string);
}

struct Atom {
  Op op;
  Func builtinFunc;
  Type type;
  double val;
  int argIndex; // 该原子是函数的第几个参数

  Line *line;         // 原子关联的行
  std::string symbol; // 原子关联的符号

  Atom *prototype;          // 原子调用函数的原型
  Atom *callee;             // 直接调用该原子的原子
  std::vector<Atom *> refs; // 通过引用等方式调用该原子的原子
  std::vector<Atom *> args;
  std::vector<std::string> formalArgs;

  void removeArg(Atom *arg);
  void removeArg0();
  void setCallee(Atom *callee);
  void setPrototype(Atom *prototype);
  void addReferree(Atom *referree);
  void setArgsBy(std::stack<Atom *> &stack, int num = -1);
  void setArgsBy(std::queue<Atom *> &queue, int num = -1);
  void setFunctionArg(std::vector<double> &argVals);
  void getFunctionVal(std::vector<Atom *> args, Atom *callee);
  void updateVal(); // 更新本原子的数值
  void update(bool updateFunction = false,
              int level = 0); // 自底向上更新所有关联原子
  std::vector<Token> getTokens(Op callerOp = If);
  std::string getExpressionCode(Line *line, Op callerOp = If);
  std::string getLineCode();
  std::string getNumberString();
  std::string getReferenceString(Atom *lineAtom = nullptr);

  Atom()
      : op{Number}, builtinFunc{NoBuiltin}, type{Decimal}, val{0.0},
        argIndex{0}, line{nullptr}, symbol{""}, prototype{nullptr},
        callee{nullptr}, refs{}, args{}, formalArgs{} {};
};

struct Line {
  Atom *atom;
  int index;
  bool visible;
  std::vector<Token> tokens;

  Line(Atom *atom)
      : atom{atom}, index{0}, visible{true}, tokens{atom->getTokens()} {};
};

struct Page {
  std::vector<Atom *> atoms;
  std::vector<Line *> lines;
  std::map<std::string, Atom *> symbols;
  std::string err, filename;
  bool dirty;

  Atom *createAtom(Op op);
  void createLine(std::string input);
  void editLine(int index, std::string input);
  void addSymbol(int lineIndex, std::string symbol);
  std::string getCodes();

  Page() : err{""}, filename{""}, dirty{false} {};
  ~Page();
};

#endif
