# SPIM S20 MIPS simulator.
# CMake build for the command-line simulator (spim) and the Qt interface
# (QtSpim).
#
# Copyright (c) 2026, qtspim-arm contributors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# Neither the name of the James R. Larus nor the names of its contributors may be
# used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# See docs/BUILDING.md.  The qmake project (QtSpim/QtSpim.pro) and
# spim/Makefile are kept for now; this file builds the same programs with
# the same parser/scanner options and the same compile-time definitions.

cmake_minimum_required(VERSION 3.21)

# On macOS, build a Universal Binary (Apple Silicon and Intel) unless the
# architectures are given explicitly.  This must be set before project().
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING
      "macOS architectures to build (default: Universal Binary)")
endif()

# Minimum macOS version: that of Qt 6.11, the Qt used for releases.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET
   AND NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
endif()

project(QtSpim LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

option(SPIM_BUILD_QTSPIM "Build the QtSpim application (requires Qt 6)" ON)

# Application identity (docs/PORTING_PLAN.md, owner's decisions).  Releases
# are tagged v<QTSPIM_VERSION>-arm.<QTSPIM_BUILD_NUMBER>; the release build
# sets QTSPIM_BUILD_NUMBER, and 0 means a development build.
set(QTSPIM_APP_NAME "QtSpim ARM")
set(QTSPIM_BUNDLE_ID "net.yuan-tw.qtspim")
set(QTSPIM_VERSION "9.1.24")
set(QTSPIM_BUILD_NUMBER "0" CACHE STRING
  "Build number (CFBundleVersion and the N in v9.1.24-arm.N); 0 = development")
if(QTSPIM_BUILD_NUMBER STREQUAL "0")
  set(QTSPIM_FULL_VERSION "${QTSPIM_VERSION}-arm (development build)")
else()
  set(QTSPIM_FULL_VERSION "${QTSPIM_VERSION}-arm.${QTSPIM_BUILD_NUMBER}")
endif()
option(SPIM_BUILD_HELP "Build the QtSpim help files with qhelpgenerator" ON)

# ---------------------------------------------------------------------------
# Parser and scanner
# ---------------------------------------------------------------------------

# The bison that comes with macOS (2.3) is too old.  Prefer Homebrew's
# keg-only bison and flex, which are not on the PATH by default.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  foreach(prefix /opt/homebrew/opt /usr/local/opt)
    foreach(pkg bison flex)
      if(IS_DIRECTORY "${prefix}/${pkg}/bin")
        list(APPEND CMAKE_PROGRAM_PATH "${prefix}/${pkg}/bin")
      endif()
    endforeach()
  endforeach()
endif()

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.5.35 REQUIRED)

set(SPIM_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${SPIM_GENERATED_DIR})

# Same options and file names as spim/Makefile and QtSpim/QtSpim.pro:
#   bison -d --defines=parser_yacc.h --output=parser_yacc.cpp -p yy parser.y
#   flex -I -8 -Pyy scanner.l
bison_target(spim_parser CPU/parser.y ${SPIM_GENERATED_DIR}/parser_yacc.cpp
  DEFINES_FILE ${SPIM_GENERATED_DIR}/parser_yacc.h
  COMPILE_FLAGS "-p yy")
flex_target(spim_scanner CPU/scanner.l ${SPIM_GENERATED_DIR}/lex.yy.cpp
  COMPILE_FLAGS "-I -8 -Pyy")
add_flex_bison_dependency(spim_scanner spim_parser)

set(SPIM_CORE_SOURCES
  CPU/data.cpp
  CPU/display-utils.cpp
  CPU/inst.cpp
  CPU/mem.cpp
  CPU/run.cpp
  CPU/spim-utils.cpp
  CPU/string-stream.cpp
  CPU/sym-tbl.cpp
  CPU/syscall.cpp
  ${BISON_spim_parser_OUTPUT_SOURCE}
  ${FLEX_spim_scanner_OUTPUTS})

# The simulator core passes string literals as char *.
set(SPIM_WARNING_FLAGS
  "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wno-write-strings>")

# ---------------------------------------------------------------------------
# spim: command-line simulator
# ---------------------------------------------------------------------------

set(SPIM_EXCEPTION_HANDLER "${CMAKE_INSTALL_FULL_DATADIR}/spim/exceptions.s"
  CACHE STRING "Default exception handler file compiled into spim")

add_executable(spim spim/spim.cpp ${SPIM_CORE_SOURCES})
target_include_directories(spim PRIVATE CPU ${SPIM_GENERATED_DIR})
target_compile_definitions(spim PRIVATE
  # Memory sizes from spim/Makefile (MEM_SIZES).  QtSpim uses the defaults
  # in CPU/spim.h, as in QtSpim.pro.
  TEXT_SIZE=65536
  DATA_SIZE=131072
  K_TEXT_SIZE=65536
  "DEFAULT_EXCEPTION_HANDLER=\"${SPIM_EXCEPTION_HANDLER}\"")
target_compile_options(spim PRIVATE ${SPIM_WARNING_FLAGS})
if(UNIX AND NOT APPLE)
  target_link_libraries(spim PRIVATE m)
endif()

install(TARGETS spim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES CPU/exceptions.s DESTINATION ${CMAKE_INSTALL_DATADIR}/spim)
install(FILES Documentation/spim.man DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
  RENAME spim.1)

enable_testing()
add_test(NAME golden
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-tests.sh $<TARGET_FILE:spim>)

# ---------------------------------------------------------------------------
# QtSpim: Qt interface
# ---------------------------------------------------------------------------

if(NOT SPIM_BUILD_QTSPIM)
  return()
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets PrintSupport)

# A Universal Binary needs a Universal Qt.  Homebrew's Qt is built for the
# host architecture only; the Qt online installer provides Universal builds.
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
  get_target_property(_qtcore_location Qt6::Core LOCATION)
  if(_qtcore_location AND EXISTS "${_qtcore_location}")
    execute_process(COMMAND lipo -archs "${_qtcore_location}"
      OUTPUT_VARIABLE _qtcore_archs OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _lipo_result ERROR_QUIET)
    if(_lipo_result EQUAL 0)
      separate_arguments(_qtcore_archs)
      foreach(_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        if(NOT _arch IN_LIST _qtcore_archs)
          message(FATAL_ERROR
            "QtSpim is being built for ${CMAKE_OSX_ARCHITECTURES}, but the Qt "
            "found at ${_qtcore_location} only contains: ${_qtcore_archs}.\n"
            "Use a Universal Qt (for example from the Qt online installer), or "
            "build for the Qt architecture only, e.g. "
            "-DCMAKE_OSX_ARCHITECTURES=arm64.  See docs/BUILDING.md.")
        endif()
      endforeach()
    endif()
  endif()
endif()

qt_add_executable(QtSpim WIN32 MACOSX_BUNDLE
  QtSpim/main.cpp
  QtSpim/spimview.cpp
  QtSpim/menu.cpp
  QtSpim/regwin.cpp
  QtSpim/textwin.cpp
  QtSpim/datawin.cpp
  QtSpim/state.cpp
  QtSpim/console.cpp
  QtSpim/spim_support.cpp
  QtSpim/spimview.h
  QtSpim/regtextedit.h
  QtSpim/texttextedit.h
  QtSpim/datatextedit.h
  QtSpim/spim_settings.h
  QtSpim/settablecheckbox.h
  QtSpim/console.h
  QtSpim/spimview.ui
  QtSpim/savelogfile.ui
  QtSpim/printwindows.ui
  QtSpim/runparams.ui
  QtSpim/settings.ui
  QtSpim/changevalue.ui
  QtSpim/breakpoint.ui
  QtSpim/windows_images.qrc
  QtSpim/exception.qrc
  ${SPIM_CORE_SOURCES})

set_target_properties(QtSpim PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON)
target_include_directories(QtSpim PRIVATE CPU QtSpim ${SPIM_GENERATED_DIR})
target_compile_definitions(QtSpim PRIVATE
  "QTSPIM_ARM_VERSION=\"${QTSPIM_FULL_VERSION}\"")
target_compile_options(QtSpim PRIVATE ${SPIM_WARNING_FLAGS})
target_link_libraries(QtSpim PRIVATE Qt6::Core Qt6::Widgets Qt6::PrintSupport)

if(APPLE)
  set_source_files_properties(QtSpim/NewIcon.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
  target_sources(QtSpim PRIVATE QtSpim/NewIcon.icns)
  # QtSpim ARM.app, so it can be installed next to the original QtSpim.app.
  # QtSpim/macinfo.plist is still used by QtSpim.pro.
  configure_file(QtSpim/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
  set_target_properties(QtSpim PROPERTIES
    OUTPUT_NAME "${QTSPIM_APP_NAME}"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
elseif(WIN32)
  target_sources(QtSpim PRIVATE QtSpim/qtspim.rc)
endif()

install(TARGETS QtSpim
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---------------------------------------------------------------------------
# Help files (qtspim.qch and the qtspim.qhc collection for Qt Assistant)
# ---------------------------------------------------------------------------

if(NOT SPIM_BUILD_HELP)
  return()
endif()

find_package(Qt6 QUIET COMPONENTS ToolsTools)
if(NOT TARGET Qt6::qhelpgenerator)
  message(WARNING "qhelpgenerator not found: the QtSpim help files are not built. "
    "Install the Qt tools or configure with -DSPIM_BUILD_HELP=OFF.")
  return()
endif()

# qhelpgenerator writes qtspim.qch next to qtspim.qhcp and resolves
# qtspim.qhcp's ../windows_images/NewIcon.ico relative to it, so the help
# sources are copied into the build tree and generated there.
set(help_src ${CMAKE_CURRENT_SOURCE_DIR}/QtSpim/help)
set(help_dir ${CMAKE_CURRENT_BINARY_DIR}/help/help)
file(GLOB help_inputs CONFIGURE_DEPENDS
  ${help_src}/*.html ${help_src}/*.jpg)
list(APPEND help_inputs ${help_src}/qtspim.qhp ${help_src}/qtspim.qhcp)

add_custom_command(
  OUTPUT ${help_dir}/qtspim.qch ${help_dir}/qtspim.qhc
  COMMAND ${CMAKE_COMMAND} -E make_directory ${help_dir} ${help_dir}/../windows_images
  COMMAND ${CMAKE_COMMAND} -E rm -f ${help_dir}/qtspim.qch ${help_dir}/qtspim.qhc
  COMMAND ${CMAKE_COMMAND} -E copy ${help_inputs} ${help_dir}
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/QtSpim/windows_images/NewIcon.ico
    ${help_dir}/../windows_images/
  COMMAND Qt6::qhelpgenerator qtspim.qhcp -o qtspim.qhc
  WORKING_DIRECTORY ${help_dir}
  DEPENDS ${help_inputs} ${CMAKE_CURRENT_SOURCE_DIR}/QtSpim/windows_images/NewIcon.ico
  COMMENT "Building QtSpim help (qtspim.qch, qtspim.qhc)"
  VERBATIM)

set(help_outputs ${help_dir}/qtspim.qch ${help_dir}/qtspim.qhc)
if(APPLE)
  # QtSpim looks for the collection in QtSpim.app/Contents/Resources/doc.
  set_source_files_properties(${help_outputs} PROPERTIES
    GENERATED TRUE
    MACOSX_PACKAGE_LOCATION Resources/doc)
  target_sources(QtSpim PRIVATE ${help_outputs})
else()
  add_custom_target(qtspim_help ALL DEPENDS ${help_outputs})
  install(FILES ${help_outputs} DESTINATION ${CMAKE_INSTALL_DATADIR}/qtspim/help)
endif()
