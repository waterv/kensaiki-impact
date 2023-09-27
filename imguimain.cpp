#include "impl.h"
#include "main.h"

#define ID(str, primaryKey) (~format(str "##{}", (ul)(primaryKey)))
#define VID(str, primaryKey) (~format("{}##{}", (str), (ul)(primaryKey)))
#define opSymbol(op) (showOpCode ? OpCodeStr[op] : OpStr[op])

#define strClear(str) strcpy(str, "")
#define strEmpty(str) ((str)[0] == '\0')

#define OpenPopupWithAtom(label)                                               \
  do {                                                                         \
    activeAtom = tokenAtom;                                                    \
    activeLineAtom = lineAtom;                                                 \
    ImGui::OpenPopup(label);                                                   \
  } while (0)
#define SelectableNewLine(counter)                                             \
  if (++counter % 5)                                                           \
    ImGui::SameLine();
#define UpdateIntInputByAtomVal(atom)                                          \
  strcpy(intInput, ~format(TypeFmt[(int)(atom)->type], (int)(atom)->val))
#define UpdateActiveAtom()                                                     \
  do {                                                                         \
    activeAtom->update(isFunction(activeLineAtom));                            \
    page->dirty = true;                                                        \
  } while (0)

using ul = unsigned long;
using std::erase, fmt::format, fmt::join;
using std::ifstream, std::ofstream, std::runtime_error;
using std::map, std::string, std::vector, std::pair;

bool showOpCode = false, showDemoWindow = false, showExpressionTree = false;
extern int precision, OpArgNum[];
extern string OpStr[], OpCodeStr[], TypeStr[], TypeFmt[], BuiltinFuncStr[];
extern map<string, pair<int, Func>> BuiltinFuncs;

const char *operator~(const string &str) { return str.c_str(); }
void bulletList(Atom *atom, Atom *line);

int main() {
  GLFWwindow *window = init();
  srand(time(nullptr));
  ImGui::GetStyle().FrameRounding = 12.0f;

  vector<Page *> pages{new Page};
  Atom *activeAtom = nullptr, *activeLineAtom = nullptr;

  mainLoop {
    newFrame();

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Kensaiki Impact", nullptr, WindowFlags);

    if (ImGui::BeginTabBar("MainTabBar", TabBarFlags)) {
      Page *currentPage;

      for (auto page : pages) {
        bool open = true;
        static char intInput[64] = "";

        if (ImGui::BeginTabItem(
                VID(!strEmpty(page->filename) ? page->filename : "New Page",
                    page),
                pages.size() > 1 ? &open : nullptr,
                page->dirty ? ImGuiTabItemFlags_UnsavedDocument : 0)) {
          currentPage = page;

          static char input[1024] = "";
          bool isExpressionEnter =
              ImGui::InputText("##Expression", input, 1024, InputTextFlags);

          ImGui::SameLine();

          if (ImGui::Button(ID("Enter", page)) || isExpressionEnter) {
            try {
              page->createLine(string{input});
              page->err = "";
              page->dirty = true;
              strClear(input);
            } catch (runtime_error &e) {
              page->err = e.what();
            }
          }

          if (!strEmpty(page->err))
            ImGui::TextColored(ErrTextColor, "[Error] %s", ~page->err);

          ImGui::Separator();

          if (ImGui::BeginTable(ID("MainFrame", page), 4, TableFlags)) {
            ImGui::TableSetupColumn(ID("No", page),
                                    ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn(ID("Identifier", page),
                                    ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(ID("Expression", page),
                                    ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(ID("Result", page),
                                    ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();

            for (auto line : page->lines) {
              if (!line->visible)
                continue;

              auto lineAtom = line->atom;
              ImGui::TableNextRow();

              // Line No.
              ImGui::TableSetColumnIndex(0);
              ImGui::Text("%d", line->index);

              // Identifier
              ImGui::TableSetColumnIndex(1);
              {
                if (isFunction(lineAtom))
                  ImGui::Text("%s", ~format("{}({})", lineAtom->symbol,
                                            join(lineAtom->formalArgs, ", ")));
                else if (isVariable(lineAtom))
                  ImGui::Text("%s", ~lineAtom->symbol);
              }

              // Expression
              ImGui::TableSetColumnIndex(2);
              {
                for (auto token : line->tokens) {
                  auto tokenAtom = token.second;
                  if (tokenAtom && isNumberLiteral(tokenAtom)) {
                    if (ImGui::SmallButton(
                            VID(tokenAtom->getNumberString(), tokenAtom))) {
                      UpdateIntInputByAtomVal(tokenAtom);
                      OpenPopupWithAtom("ChangeAtomVal");
                    }
                  } else if (tokenAtom && isReference(tokenAtom)) {
                    if (ImGui::SmallButton(
                            VID(tokenAtom->getReferenceString(lineAtom),
                                tokenAtom)))
                      OpenPopupWithAtom("ChangeRefStr");
                  } else if (tokenAtom && isUnaryOperator(tokenAtom)) {
                    if (ImGui::SmallButton(
                            VID(opSymbol(tokenAtom->op), tokenAtom)))
                      OpenPopupWithAtom("ChangeAtomUnaOp");
                  } else if (tokenAtom && isBinaryOperator(tokenAtom)) {
                    if (ImGui::SmallButton(
                            VID(opSymbol(tokenAtom->op), tokenAtom)))
                      OpenPopupWithAtom("ChangeAtomBinOp");
                  } else {
                    ImGui::Text("%s", ~token.first);
                  }
                  ImGui::SameLine();
                }
              }

              // Result
              ImGui::TableSetColumnIndex(3);
              {
                if (!isFunction(line->atom))
                  ImGui::Text("%s", ~line->atom->getNumberString());
              }
            }

            if (ImGui::BeginPopup("ChangeAtomVal")) {
              // Base Selection
              int j = (int)activeAtom->type;
              for (int i = 0; i < (int)Boolean; i++) {
                if (ImGui::Selectable(~TypeStr[i], j == i, SelectableFlags,
                                      SelectableSize)) {
                  activeAtom->type = (Type)i;
                  UpdateActiveAtom();
                  UpdateIntInputByAtomVal(activeAtom);
                }
                ImGui::SameLine();
              }
              ImGui::NewLine();
              ImGui::Separator();

              // Decimal Input
              if (ImGui::InputDouble("##AtomVal", &activeAtom->val)) {
                UpdateIntInputByAtomVal(activeAtom);
                UpdateActiveAtom();
              }

              // Base 2/8/16 Input
              if (isBaseInputDisabled(activeAtom))
                ImGui::BeginDisabled();
              if (ImGui::InputText("##AtomValInt", intInput, 64)) {
                activeAtom->val = (double)ToDecimalInteger(intInput);
                UpdateActiveAtom();
              }
              if (isBaseInputDisabled(activeAtom))
                ImGui::EndDisabled();

              ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("ChangeRefStr")) {
              static char refInput[1024] = "";
              bool isRefStrEnter =
                  ImGui::InputText("##RefStr", refInput, 1024, InputTextFlags);

              ImGui::SameLine();

              if (ImGui::Button("OK") || isRefStrEnter) {
                if (BuiltinFuncs.contains(refInput)) {
                  page->err = format("redefinition of keyword '{}'", refInput);
                } else if (!isIdentifierDefined(page, refInput)) {
                  page->err = format("undefined identifier '{}'", refInput);
                } else if (!isVariable(page->symbols[refInput])) {
                  page->err = format("'{}' is not a variable", refInput);
                } else {
                  page->err = "";
                  activeAtom->removeArg0();
                  activeAtom->argIndex = 0;
                  page->symbols[refInput]->addReferree(activeAtom);
                  strClear(refInput);
                  UpdateActiveAtom();
                }
              }

              if (isFunction(activeLineAtom)) {
                ImGui::Separator();
                int j = 0, k = 0;
                for (auto arg : activeLineAtom->formalArgs) {
                  if (ImGui::Selectable(~arg,
                                        isFunctionArgument(activeAtom) &&
                                            (activeAtom->argIndex == j),
                                        SelectableFlags, SelectableSize)) {
                    activeAtom->removeArg0();
                    activeAtom->argIndex = j;
                    UpdateActiveAtom();
                  }
                  j += 1;
                  SelectableNewLine(k);
                }
              }

              ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("ChangeAtomUnaOp")) {
              int j = (int)activeAtom->op;
              for (int i = 0, k = 0; i < (int)OpCount; i++) {
                if (OpArgNum[i] != 1)
                  continue;
                if (ImGui::Selectable(~opSymbol(i), j == i, SelectableFlags,
                                      SelectableSize)) {
                  activeAtom->op = (Op)i;
                  UpdateActiveAtom();
                }
                SelectableNewLine(k);
              }
              ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("ChangeAtomBinOp")) {
              int j = (int)activeAtom->op;
              for (int i = 0, k = 0; i < (int)OpCount; i++) {
                if (OpArgNum[i] != 2)
                  continue;
                if (ImGui::Selectable(~opSymbol(i), j == i, SelectableFlags,
                                      SelectableSize)) {
                  activeAtom->op = (Op)i;
                  UpdateActiveAtom();
                }
                SelectableNewLine(k);
              }
              ImGui::EndPopup();
            }

            ImGui::EndTable();
          }

          if (showExpressionTree && activeLineAtom) {
            ImGui::Separator();
            ImGui::Text("%s", ~activeLineAtom->getLineCode());
            bulletList(activeLineAtom, activeLineAtom);
          }

          ImGui::EndTabItem();
        }

        if (!open) {
          if (page->dirty)
            ImGui::OpenPopup(ID("Delete?", page));
          else
            erase(pages, page);
        }

        if (currentPage == page) {
          if (ImGui::TabItemButton("=", TabBarLeadingFlags))
            ImGui::OpenPopup("FileOperation");

          if (ImGui::BeginPopup("FileOperation")) {
            ImGui::MenuItem("File", nullptr, false, false);

            if (ImGui::MenuItem("New"))
              pages.push_back(new Page);

            if (ImGui::BeginMenu("Open...")) {
              static char filenameInput[1024] = "";
              bool isFilenameEnter = ImGui::InputText(
                  "##Filename", filenameInput, 1024, InputTextFlags);
              ImGui::SameLine();
              if (ImGui::Button("Open") || isFilenameEnter) {
                ImGui::CloseCurrentPopup();
                auto newPage = new Page;
                try {
                  ifstream ifs{filenameInput};
                  string str;
                  while (getline(ifs, str))
                    newPage->createLine(str);
                } catch (runtime_error &e) {
                  newPage->err = e.what();
                }
                newPage->filename = string{filenameInput};
                pages.push_back(newPage);
                strClear(filenameInput);
              }
              ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Save", nullptr, false,
                                !strEmpty(page->filename))) {
              ofstream ofs{page->filename};
              ofs << page->getCodes();
              page->dirty = false;
            }

            if (ImGui::BeginMenu("Save As...")) {
              static char filenameInput[1024] = "";
              bool isFilenameEnter = ImGui::InputText(
                  "##Filename", filenameInput, 1024, InputTextFlags);
              ImGui::SameLine();
              if (ImGui::Button("Save") || isFilenameEnter) {
                ImGui::CloseCurrentPopup();
                ofstream ofs{filenameInput};
                ofs << page->getCodes();
                page->filename = string{filenameInput};
                page->dirty = false;
                strClear(filenameInput);
              }
              ImGui::EndMenu();
            }

            ImGui::Separator();
            ImGui::MenuItem("Settings", nullptr, false, false);

            if (ImGui::BeginMenu("Display Precision")) {
              ImGui::DragInt("##precision", &precision, 1, 0, 9, "%d",
                             ImGuiSliderFlags_AlwaysClamp);
              ImGui::EndMenu();
            }

            ImGui::MenuItem("Show Code for Operators", nullptr, &showOpCode);

            ImGui::Separator();
            ImGui::MenuItem("Debug", nullptr, false, false);
            ImGui::MenuItem("Show Demo Window", nullptr, &showDemoWindow);
            ImGui::MenuItem("Show Expression Tree", nullptr,
                            &showExpressionTree);

            ImGui::EndPopup();
          }
        }

        if (ImGui::BeginPopupModal(ID("Delete?", page), NULL,
                                   PopupModalFlags)) {
          ImGui::Text("This operation cannot be undone!");
          ImGui::Separator();
          if (ImGui::Button("OK", {120, 0})) {
            ImGui::CloseCurrentPopup();
            erase(pages, page);
          }
          ImGui::SetItemDefaultFocus();
          ImGui::SameLine();
          if (ImGui::Button("Cancel", {120, 0}))
            ImGui::CloseCurrentPopup();
          ImGui::EndPopup();
        }
      }
      ImGui::EndTabBar();
    }

    ImGui::End();

    if (showDemoWindow)
      ImGui::ShowDemoWindow(&showDemoWindow);

    render(window);
  }

  cleanup(window);
  return 0;
}

void bulletList(Atom *atom, Atom *line) {
  if (!atom)
    return;
  ImGui::Bullet();
  if (isNumberLiteral(atom)) {
    ImGui::SmallButton(~atom->getNumberString());
    ImGui::SameLine();
    ImGui::Text("Literal");
  } else if (isVariableReference(atom)) {
    ImGui::SmallButton(~atom->getReferenceString(line));
    ImGui::SameLine();
    ImGui::Text("Variable");
    ImGui::SameLine();
    ImGui::TextDisabled("(-> %p)", atom->args[0]);
  } else if (isFunctionArgument(atom)) {
    ImGui::SmallButton(~atom->getReferenceString(line));
    ImGui::SameLine();
    ImGui::Text("Argument");
  } else if (atom->op == FuncCall) {
    if (atom->builtinFunc == NoBuiltin) {
      ImGui::SmallButton(~atom->prototype->symbol);
      ImGui::SameLine();
      ImGui::Text("Function");
      ImGui::SameLine();
      ImGui::TextDisabled("(-> %p)", atom->prototype);
    } else {
      ImGui::SmallButton(~BuiltinFuncStr[atom->builtinFunc]);
      ImGui::SameLine();
      ImGui::Text("Builtin Function");
    }
  } else {
    ImGui::SmallButton(~OpCodeStr[atom->op]);
    ImGui::SameLine();
    ImGui::Text("Operator");
  }
  ImGui::SameLine();
  ImGui::TextDisabled("%p", atom);
  ImGui::Indent(50.0f);
  for (auto u : atom->args)
    bulletList(u, line);
  ImGui::Unindent(50.0f);
}
