cmake_minimum_required(VERSION 3.20)
project(pptxlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options -----------------------------------------------------------
option(PPTXLIB_BUILD_TESTS "Build unit tests" ON)
option(PPTXLIB_ENABLE_SANITIZERS "Build with ASan/UBSan" ON)
option(PPTXLIB_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(pptxlib_warnings INTERFACE)
if (MSVC)
    target_compile_options(pptxlib_warnings INTERFACE /W4)
else()
    target_compile_options(pptxlib_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()
if (PPTXLIB_WARNINGS_AS_ERRORS)
    if (MSVC)
        target_compile_options(pptxlib_warnings INTERFACE /WX)
    else()
        target_compile_options(pptxlib_warnings INTERFACE -Werror)
    endif()
endif()

add_library(pptxlib_sanitizers INTERFACE)
if (PPTXLIB_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(pptxlib_sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(pptxlib_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

# --- Dependencies --------------------------------------------------------
# NOTE: Nothing is wired in yet. Once Phase 0 (see ARCHITECTURE.md) picks the
# XML parser and ZIP library, add them here — e.g. via FetchContent or
# find_package. Do not hand-vendor sources without recording the license in
# THIRD_PARTY_LICENSES.md.
#
# include(FetchContent)
# FetchContent_Declare(pugixml GIT_REPOSITORY ... GIT_TAG ...)
# FetchContent_MakeAvailable(pugixml)

# --- Library ---------------------------------------------------------------
# NOTE: src/ is currently empty. Phase 1 adds package/, xml/, relationships/,
# and model/ sources here per the master architecture doc.
add_library(pptxlib
    # src/package/... .cpp
)
target_include_directories(pptxlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(pptxlib PRIVATE pptxlib_warnings pptxlib_sanitizers)
set_target_properties(pptxlib PROPERTIES LINKER_LANGUAGE CXX)

# --- Tests -------------------------------------------------------------
if (PPTXLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
