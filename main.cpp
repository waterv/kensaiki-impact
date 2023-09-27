#include "main.h"
#include <cmath>
#include <cstdio>

#define ArgT(n) (this->args[n]->type)
#define ArgV(n) (this->args[n]->val)
#define CaseTypeVal(C, T, V)                                                   \
  case C:                                                                      \
    this->type = T;                                                            \
    this->val = V;                                                             \
    break
#define MergeArgTokens(arg)                                                    \
  do {                                                                         \
    auto tokens = (arg)->getTokens(this->op);                                  \
    res.insert(res.end(), tokens.begin(), tokens.end());                       \
  } while (0)
#define ArgCode(arg) ((arg)->getExpressionCode(line, this->op))
#define BothOrDec (ArgT(0) == ArgT(1) ? ArgT(0) : Decimal)

#define Eps (1e-9)
#define ls(a, b) ((a) - (b) < -Eps)
#define gr(a, b) ((a) - (b) > Eps)
#define eq(a, b) (abs((a) - (b)) < Eps)
#define ge(a, b) (!ls(a, b))
#define le(a, b) (!gr(a, b))
#define ne(a, b) (!eq(a, b))

#define RecurMaxLevel 256

using std::erase, fmt::format, fmt::join;
using std::map, std::string, std::vector, std::pair;
extern Atom *parseLine(Page *pg, const string &str);

int precision = 9;
int OpArgNum[] = {-2, -1, 0, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
                  2,  2,  2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 3, -3};
int OpPriority[] = {0, 0, 0, 1, 2, 2, 2, 3, 3, 3,  3,  4,  4,  5, 5,
                    6, 7, 8, 9, 9, 9, 9, 9, 9, 10, 11, 12, 13, 0};
string OpStr[] = {"",    "",  "",  "^",  "−",  "+",   "~",   "×",  "÷R", "÷",
                  "mod", "+", "−", "<<", ">>", "and", "xor", "or", "<",  ">",
                  "=",   "≥", "≤", "≠",  "¬",  "∧",   "∨",   "",   ""};
string OpCodeStr[] = {"",  "",   "",   "**", "-",  "+",  "~",  "*",  "//", "/",
                      "%", "+",  "-",  "<<", ">>", "&",  "^",  "|",  "<",  ">",
                      "=", ">=", "<=", "!=", "!",  "&&", "||", "?:", ""};
string TypeStr[] = {"DEC", "BIN", "OCT", "HEX"};
string TypeFmt[] = {"{}", "0b{:B}", "0o{:o}", "0x{:X}"};
map<string, pair<int, Func>> BuiltinFuncs = {
    {"abs", {1, Abs}},         {"max", {2, Max}},
    {"min", {2, Min}},         {"sqrt", {1, Sqrt}},
    {"cbrt", {1, Cbrt}},       {"sin", {1, Sin}},
    {"cos", {1, Cos}},         {"tan", {1, Tan}},
    {"tg", {1, Tan}},          {"arcsin", {1, ArcSin}},
    {"arccos", {1, ArcCos}},   {"arctan", {1, ArcTan}},
    {"arctg", {1, ArcTan}},    {"asin", {1, ArcSin}},
    {"acos", {1, ArcCos}},     {"atan", {1, ArcTan}},
    {"atg", {1, ArcTan}},      {"sinh", {1, Sinh}},
    {"cosh", {1, Cosh}},       {"tanh", {1, Tanh}},
    {"tgh", {1, Tanh}},        {"arcsinh", {1, ArcSinh}},
    {"arccosh", {1, ArcCosh}}, {"arctanh", {1, ArcTanh}},
    {"arctgh", {1, ArcTanh}},  {"asinh", {1, ArcSinh}},
    {"acosh", {1, ArcCosh}},   {"atanh", {1, ArcTanh}},
    {"atgh", {1, ArcTanh}},    {"erf", {1, Erf}},
    {"erfc", {1, Erfc}},       {"exp", {1, Exp}},
    {"ln", {1, Ln}},           {"loge", {1, Ln}},
    {"lg", {1, Lg}},           {"log10", {1, Lg}},
    {"lb", {1, Lb}},           {"log2", {1, Lb}},
    {"log", {2, Log}},         {"floor", {1, Floor}},
    {"ceil", {1, Ceil}},       {"ceiling", {1, Ceil}},
    {"round", {1, Round}},     {"trunc", {1, Trunc}},
    {"int", {1, Trunc}},       {"gamma", {1, Gamma}},
    {"toDec", {1, ToDec}},     {"toBin", {1, ToBin}},
    {"toOct", {1, ToOct}},     {"toHex", {1, ToHex}},
    {"is", {1, Is}},           {"toBool", {1, Is}},
    {"isInf", {1, IsInf}},     {"isNan", {1, IsNan}},
    {"random", {1, Random}},   {"gcd", {2, Gcd}},
    {"lcm", {2, Lcm}},         {"rad", {1, Rad}},
    {"radius", {1, Rad}},      {"deg", {1, Deg}},
    {"degree", {1, Deg}}};
string BuiltinFuncStr[] = {
    "",      "abs",     "max",     "min",     "sqrt",   "cbrt",  "sin",
    "cos",   "tan",     "arcsin",  "arccos",  "arctan", "sinh",  "cosh",
    "tanh",  "arcsinh", "arccosh", "arctanh", "erf",    "erfc",  "exp",
    "ln",    "lg",      "lb",      "log",     "floor",  "ceil",  "round",
    "trunc", "gamma",   "toDec",   "toBin",   "toOct",  "toHex", "is",
    "isInf", "isNan",   "random",  "gcd",     "lcm",    "rad",   "deg"};

void Atom::removeArg(Atom *arg) {
  erase(this->args, arg);
  erase(arg->refs, this);
}

void Atom::removeArg0() {
  if (!this->args.empty())
    this->removeArg(this->args[0]);
}

void Atom::setCallee(Atom *callee) {
  callee->args.push_back(this);
  this->refs.push_back(callee);
  this->callee = callee;
}

void Atom::setPrototype(Atom *prototype) {
  this->prototype = prototype;
  prototype->refs.push_back(this);
}

void Atom::addReferree(Atom *referree) {
  referree->args.push_back(this);
  this->refs.push_back(referree);
}

void Atom::setArgsBy(std::stack<Atom *> &stack, int num) {
  std::stack<Atom *> stack2;
  while (!stack.empty() && num--) {
    stack2.push(stack.top());
    stack.pop();
  }
  while (!stack2.empty()) {
    stack2.top()->setCallee(this);
    stack2.pop();
  }
}

void Atom::setArgsBy(std::queue<Atom *> &queue, int num) {
  while (!queue.empty() && num--) {
    queue.front()->setCallee(this);
    queue.pop();
  }
}

void Atom::setFunctionArg(vector<double> &argVals) {
  if (isReference(this)) {
    if (isFunctionArgument(this)) {
      this->val = argVals[this->argIndex];
      this->update();
    }
  } else {
    for (auto arg : this->args)
      arg->setFunctionArg(argVals);
  }
}

void Atom::getFunctionVal(vector<Atom *> args, Atom *callee) {
  vector<double> argVals{};
  for (auto arg : args)
    argVals.push_back(arg->val);
  this->setFunctionArg(argVals);
  callee->val = this->val;
  callee->type = this->type;
}

void Atom::updateVal() {
  switch (this->op) {
  case Number:
  default:
    break;
  case Reference:
    if (isFunctionArgument(this))
      break;
    this->type = ArgT(0);
    this->val = ArgV(0);
    break;
  case FuncCall:
    switch (this->builtinFunc) {
    default:
      break;
    case NoBuiltin:
      prototype->getFunctionVal(this->args, this);
      break;

      CaseTypeVal(Abs, ArgT(0), abs(ArgV(0)));
      CaseTypeVal(Min, ls(ArgV(0), ArgV(1)) ? ArgT(0) : ArgT(1),
                  fmin(ArgV(0), ArgV(1)));
      CaseTypeVal(Max, gr(ArgV(0), ArgV(1)) ? ArgT(0) : ArgT(1),
                  fmax(ArgV(0), ArgV(1)));
      CaseTypeVal(Sqrt, Decimal, sqrt(ArgV(0)));
      CaseTypeVal(Cbrt, Decimal, cbrt(ArgV(0)));
      CaseTypeVal(Sin, Decimal, sin(ArgV(0)));
      CaseTypeVal(Cos, Decimal, cos(ArgV(0)));
      CaseTypeVal(Tan, Decimal, tan(ArgV(0)));
      CaseTypeVal(ArcSin, Decimal, asin(ArgV(0)));
      CaseTypeVal(ArcCos, Decimal, acos(ArgV(0)));
      CaseTypeVal(ArcTan, Decimal, atan(ArgV(0)));
      CaseTypeVal(Sinh, Decimal, sinh(ArgV(0)));
      CaseTypeVal(Cosh, Decimal, cosh(ArgV(0)));
      CaseTypeVal(Tanh, Decimal, tanh(ArgV(0)));
      CaseTypeVal(ArcSinh, Decimal, asinh(ArgV(0)));
      CaseTypeVal(ArcCosh, Decimal, acosh(ArgV(0)));
      CaseTypeVal(ArcTanh, Decimal, atanh(ArgV(0)));
      CaseTypeVal(Erf, Decimal, erf(ArgV(0)));
      CaseTypeVal(Erfc, Decimal, erfc(ArgV(0)));
      CaseTypeVal(Exp, Decimal, exp(ArgV(0)));
      CaseTypeVal(Ln, Decimal, log(ArgV(0)));
      CaseTypeVal(Lg, Decimal, log10(ArgV(0)));
      CaseTypeVal(Lb, Decimal, log2(ArgV(0)));
      CaseTypeVal(Log, Decimal,
                  eq(ArgV(0), 2)    ? log2(ArgV(1))
                  : eq(ArgV(0), 10) ? log10(ArgV(1))
                                    : log(ArgV(1)) / log(ArgV(0)));
      CaseTypeVal(Floor, Decimal, floor(ArgV(0)));
      CaseTypeVal(Ceil, Decimal, ceil(ArgV(0)));
      CaseTypeVal(Round, Decimal, round(ArgV(0)));
      CaseTypeVal(Trunc, Decimal, trunc(ArgV(0)));
      CaseTypeVal(Gamma, Decimal, tgamma(ArgV(0)));
      CaseTypeVal(ToDec, Decimal, ArgV(0));
      CaseTypeVal(ToBin, Binary, (int)ArgV(0));
      CaseTypeVal(ToOct, Octal, (int)ArgV(0));
      CaseTypeVal(ToHex, Hexadecimal, (int)ArgV(0));
      CaseTypeVal(Is, Boolean, ne(ArgV(0), 0));
      CaseTypeVal(IsInf, Boolean, isinf(ArgV(0)));
      CaseTypeVal(IsNan, Boolean, isnan(ArgV(0)));
      CaseTypeVal(Random, Decimal, rand());
      CaseTypeVal(Gcd, BothOrDec, std::gcd((int)ArgV(0), (int)ArgV(1)));
      CaseTypeVal(Lcm, BothOrDec, std::lcm((int)ArgV(0), (int)ArgV(1)));
      CaseTypeVal(Rad, Decimal, ArgV(0) / 180 * acos(-1));
      CaseTypeVal(Deg, Decimal, ArgV(0) / acos(-1) * 180);
    }
    break;

    CaseTypeVal(Power, BothOrDec, pow(ArgV(0), ArgV(1)));
    CaseTypeVal(Minus, ArgT(0), -ArgV(0));
    CaseTypeVal(Plus, ArgT(0), +ArgV(0));
    CaseTypeVal(Invert, ArgT(0), ~(int)ArgV(0));
    CaseTypeVal(Multiplication, BothOrDec, ArgV(0) * ArgV(1));
    CaseTypeVal(FloorDivision, BothOrDec, (int)ArgV(0) / (int)ArgV(1));
    CaseTypeVal(Division, BothOrDec, ArgV(0) / ArgV(1));
    CaseTypeVal(Modulo, BothOrDec, fmod(ArgV(0), ArgV(1)));
    CaseTypeVal(Addition, BothOrDec, ArgV(0) + ArgV(1));
    CaseTypeVal(Subtraction, BothOrDec, ArgV(0) - ArgV(1));
    CaseTypeVal(LShifting, ArgT(0), (int)ArgV(0) << (int)ArgV(1));
    CaseTypeVal(RShifting, ArgT(0), (int)ArgV(0) >> (int)ArgV(1));
    CaseTypeVal(BitAnd, BothOrDec, (int)ArgV(0) & (int)ArgV(1));
    CaseTypeVal(BitXor, BothOrDec, (int)ArgV(0) ^ (int)ArgV(1));
    CaseTypeVal(BitOr, BothOrDec, (int)ArgV(0) | (int)ArgV(1));
    CaseTypeVal(Less, Boolean, ls(ArgV(0), ArgV(1)));
    CaseTypeVal(Greater, Boolean, gr(ArgV(0), ArgV(1)));
    CaseTypeVal(Equal, Boolean, eq(ArgV(0), ArgV(1)));
    CaseTypeVal(GEqual, Boolean, ge(ArgV(0), ArgV(1)));
    CaseTypeVal(LEqual, Boolean, le(ArgV(0), ArgV(1)));
    CaseTypeVal(NEqual, Boolean, ne(ArgV(0), ArgV(1)));
    CaseTypeVal(Not, Boolean, eq(ArgV(0), 0));
    CaseTypeVal(And, Boolean, ne(ArgV(0), 0) && ne(ArgV(1), 0));
    CaseTypeVal(Or, Boolean, ne(ArgV(0), 0) || ne(ArgV(1), 0));
    CaseTypeVal(If, ne(ArgV(0), 0) ? ArgT(1) : ArgT(2),
                ne(ArgV(0), 0) ? ArgV(1) : ArgV(2));
  }
}

void Atom::update(bool updateFunction, int level) {
  if (level >= RecurMaxLevel)
    this->val = NAN;
  else
    this->updateVal();
  if (this->line)
    this->line->tokens = getTokens();
  if (updateFunction || !isFunction(this))
    for (auto ref : this->refs)
      if (level <= RecurMaxLevel || !isnan(ref->val))
        ref->update(updateFunction, level + 1);
}

vector<Token> Atom::getTokens(Op callerOp) {
  vector<Token> res{};
  bool addBrace =
      !(OpPriority[this->op] <= OpPriority[callerOp] || callerOp == FuncCall);
  if (addBrace)
    res.push_back({"(", nullptr});
  switch (OpArgNum[this->op]) {
  case -2: // Number
    res.push_back({"", this});
    break;
  case -1: // Reference
    res.push_back({"", this});
    break;
  case 0: // FuncCall
    res.push_back({this->builtinFunc == NoBuiltin
                       ? this->prototype->symbol
                       : BuiltinFuncStr[(int)this->builtinFunc],
                   this});
    res.push_back({"(", nullptr});
    for (auto arg : this->args) {
      MergeArgTokens(arg);
      res.push_back({",", nullptr});
    }
    res.pop_back();
    res.push_back({")", nullptr});
    break;
  case 1:
    res.push_back({"", this});
    MergeArgTokens(this->args[0]);
    break;
  case 2:
    MergeArgTokens(this->args[0]);
    res.push_back({"", this});
    MergeArgTokens(this->args[1]);
    break;
  case 3: // If
    MergeArgTokens(this->args[0]);
    res.push_back({"?", nullptr});
    MergeArgTokens(this->args[1]);
    res.push_back({":", nullptr});
    MergeArgTokens(this->args[2]);
    break;
  default:
    break;
  }
  if (addBrace)
    res.push_back({")", nullptr});
  return res;
}

string Atom::getExpressionCode(Line *line, Op callerOp) {
  string res = "";
  int i = 0;
  bool addBrace =
      !(OpPriority[this->op] <= OpPriority[callerOp] || callerOp == FuncCall);
  switch (OpArgNum[this->op]) {
  case -2: // Number
    res += this->getNumberString();
    break;
  case -1: // Reference
    res += this->getReferenceString(line->atom);
    break;
  case 0: // FuncCall
    res += this->builtinFunc == NoBuiltin
               ? this->prototype->symbol
               : BuiltinFuncStr[(int)this->builtinFunc];
    for (auto arg : this->args) {
      res += (i++) ? ", " : "(";
      res += ArgCode(arg);
    }
    res += ")";
    break;
  case 1:
    res += format("{}{}", OpCodeStr[this->op], ArgCode(this->args[0]));
    break;
  case 2:
    res += format("{} {} {}", ArgCode(this->args[0]), OpCodeStr[this->op],
                  ArgCode(this->args[1]));
    break;
  case 3: // If
    res += format("{} ? {} : {}", ArgCode(this->args[0]),
                  ArgCode(this->args[1]), ArgCode(this->args[2]));
    break;
  default:
    break;
  }
  return format(addBrace ? "({})" : "{}", res);
}

string Atom::getLineCode() {
  string code;
  if (isVariable(this))
    code = format("{} := ", this->symbol);
  else if (isFunction(this))
    code = format("{} := {} => ", this->symbol, join(this->formalArgs, ", "));
  return code + this->getExpressionCode(line) + "\n";
}

string Atom::getNumberString() {
  switch (this->type) {
  case Decimal:
  default:
    return format("{}", std::stold(format("{:.{}f}", this->val, precision)));
  case Binary:
    return format("0b{:B}", (int)this->val);
  case Octal:
    return format("0o{:o}", (int)this->val);
  case Hexadecimal:
    return format("0x{:X}", (int)this->val);
  case Boolean:
    return abs(this->val) >= 1e-9 ? "True" : "False";
  }
}

string Atom::getReferenceString(Atom *lineAtom) {
  if (isVariableReference(this) || !lineAtom)
    return this->args[0]->symbol;
  return lineAtom->formalArgs[this->argIndex];
}

Atom *Page::createAtom(Op op) {
  auto atom = new Atom;
  atom->op = op;
  this->atoms.push_back(atom);
  return atom;
}

void Page::createLine(string input) {
  auto atom = parseLine(this, input);
  auto line = new Line{atom};
  this->lines.push_back(line);
  line->index = this->lines.size();
  atom->line = line;
  if (isFunction(atom) || isVariable(atom))
    this->symbols[atom->symbol] = atom;
}

void Page::editLine(int index, string input) {
  auto old = this->lines[index - 1]->atom;
  auto atom = parseLine(this, input);
  auto line = new Line{atom};
  this->lines[index - 1] = line;
  line->index = index;
  atom->line = line;
  if (isFunction(old) || isVariable(old)) {
    atom->symbol = old->symbol;
    this->symbols[old->symbol] = atom;
  }
}

void Page::addSymbol(int lineIndex, string symbol) {
  this->lines[lineIndex - 1]->atom->symbol = symbol;
  this->symbols[symbol] = this->lines[lineIndex - 1]->atom;
}

string Page::getCodes() {
  string code = "";
  for (auto line : this->lines)
    code += line->atom->getLineCode();
  return code;
}

Page::~Page() {
  for (auto atom : this->atoms)
    delete atom;
  for (auto line : this->lines)
    delete line;
}

// void printAtomGraphviz_(Atom *atom) {
//   vector<string> graphvizLabels{
//       "",     "",           "",           "**",        "-",         "+",
//       "~",    "*",          "&#47;&#47;", "&#47;",     "%",         "+",
//       "-",    "&lt;&lt;",   "&gt;&gt;",   "&amp;",     "^",         "|",
//       "&lt;", "&gt;",       "&#61;",      "&gt;&#61;", "&lt;&#61;", "!&#61;",
//       "!",    "&amp;&amp;", "||",         "?:"};
//   if (atom->op == Number)
//     printf("\"%p\" [shape=circle, label=<%lf>]\n", atom, atom->val);
//   else if (atom->op == Reference)
//     printf("\"%p\" [shape=doublecircle, label=<%s<br />%lf>]\n", atom,
//            atom->str.c_str(), atom->val);
//   else if (atom->op == FuncCall)
//     printf("\"%p\" [shape=box, label=<%s<br />%lf>]\n", atom,
//     atom->str.c_str(),
//            atom->val);
//   else
//     printf("\"%p\" [shape=box, label=<%s<br />%lf>]\n", atom,
//            graphvizLabels[atom->op].c_str(), atom->val);
//   for (auto u : atom->args) {
//     printf("\"%p\" -> \"%p\"", u, atom);
//     if (u->callee == atom)
//       printf(" [weight=3]");
//     printf("\n");
//     printAtomGraphviz_(u);
//   }
// }

// void printAtomGraphviz(Atom *atom) {
//   printf("digraph G {\n");
//   printAtomGraphviz_(atom);
//   printf("}\n");
// }
