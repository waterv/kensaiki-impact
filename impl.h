#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) &&                                 \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

const ImWchar GlyphRange[]{
    32, 255, 8722, 8722, 8743, 8744, 8800, 8800, 8804, 8805, 0,
};

constexpr ImVec4 ErrTextColor = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr ImVec2 SelectableSize = {40, 0};
constexpr int WindowFlags = ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr int TabBarFlags =
    ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs;
constexpr int TabBarLeadingFlags =
    ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip;
constexpr int InputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue;
constexpr int TableFlags = ImGuiTableFlags_Resizable;
constexpr int SelectableFlags = ImGuiSelectableFlags_DontClosePopups;
constexpr int PopupModalFlags = ImGuiWindowFlags_AlwaysAutoResize;

#define mainLoop while (!glfwWindowShouldClose(window))

GLFWwindow *init() {
  glfwSetErrorCallback(glfw_error_callback);
  assert(glfwInit() == GLFW_TRUE);

#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "Kensaiki Impact", NULL, NULL);
  assert(window != NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.Fonts->AddFontFromFileTTF("HarmonyOS_Sans_Light.ttf", 18.0f, NULL,
                               &GlyphRange[0]);

  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  return window;
}

void newFrame() {
  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void render(GLFWwindow *window) {
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(0.45f, 0.55f, 0.60f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
}

void cleanup(GLFWwindow *window) {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
}
