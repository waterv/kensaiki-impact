cmake_minimum_required(VERSION 3.11)

project(Kensaiki-Impact
  LANGUAGES CXX C
  VERSION 0.1.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

find_package(FLEX)
flex_target(Lexer
  lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.c
)

find_package(BISON)
bison_target(Parser
  parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
  VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/parser_verbose.txt
)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/misc/HarmonyOS_Sans_Light.ttf
  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY
)

file(GLOB ImGuiSrc
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.*
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.*
)

add_subdirectory(fmt)

add_executable(Kensaiki-Impact
  ${FLEX_Lexer_OUTPUTS}
  ${BISON_Parser_OUTPUTS}
  ${ImGuiSrc}
  main.cpp
  imguimain.cpp
)

set_source_files_properties(${FLEX_Lexer_OUTPUTS} PROPERTIES LANGUAGE C)
set_target_properties(Kensaiki-Impact PROPERTIES CXX_STANDARD 20 C_STANDARD 11)

target_include_directories(Kensaiki-Impact
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/libs/glfw/include
  ${CMAKE_CURRENT_SOURCE_DIR}/fmt/include

  # for "parser.h"
  ${CMAKE_CURRENT_BINARY_DIR}

  # for <GLFW/glfw3.h>
  # /usr/local/include
  # /opt/local/include
  # /opt/homebrew/include
)

find_library(OPENGL_LIBRARY OpenGL REQUIRED)
find_library(COCOA_LIBRARY Cocoa REQUIRED)
find_library(IOKIT_LIBRARY IOKit REQUIRED)
find_library(COREVID_LIBRARY CoreVideo REQUIRED)

target_link_libraries(Kensaiki-Impact
  PRIVATE
  ${OPENGL_LIBRARY}
  ${COCOA_LIBRARY}
  ${IOKIT_LIBRARY}
  ${COREVID_LIBRARY}
  /opt/homebrew/lib/libglfw.dylib
  fmt::fmt
)
