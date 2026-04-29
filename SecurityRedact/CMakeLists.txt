cmake_minimum_required(VERSION 3.16)
project(SecurityRedact LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler-Flags ────────────────────────────────────────────────────────────
if (MSVC)
    add_compile_options(/W3 /utf-8 /EHsc)
    add_compile_definitions(UNICODE _UNICODE)
else()
    # MinGW
    add_compile_options(-Wall -Wextra -municode)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# ── DLL Target ────────────────────────────────────────────────────────────────
add_library(SecurityRedact SHARED
    src/SecurityRedact.cpp
)

target_include_directories(SecurityRedact PRIVATE src)

# Windows-Libs
target_link_libraries(SecurityRedact PRIVATE
    comctl32
    user32
)

# Ausgabe direkt als SecurityRedact.dll
set_target_properties(SecurityRedact PROPERTIES
    OUTPUT_NAME "SecurityRedact"
    SUFFIX      ".dll"
    PREFIX      ""
)

# MinGW: keine lib-Prefix, statisch linken für portables .dll
if (MINGW)
    target_link_options(SecurityRedact PRIVATE -static -static-libgcc -static-libstdc++)
endif()
