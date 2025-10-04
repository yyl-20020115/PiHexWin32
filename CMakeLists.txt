cmake_minimum_required(VERSION 3.18)
project(PiHex)

enable_language(C ASM_MASM)

message(STATUS "SINCE THE ASSEMBLY FILE CONTAINS X86 INSTRUCTIONS ONLY AND YOU CAN NOT COMPILE WITH ML64.EXE!")
message(STATUS "YOU HAVE TO ALWAYS BUILD WITH WIN32 CONFIGURATION!")

# Add source files
set(SOURCES
    BLACK.ico
    PIHEX.ico
    CPUID.h
    CPUID.asm
    MAIN.h
    MAIN.c
    RESOURCES.h
    MAIN.rc
    PIHEX.c
    PIHEX.h
    POLYLOG.asm
    Polylogp5.asm
    Polylogp6.asm
)

# Add executable
add_executable(PiHex WIN32 ${SOURCES})
target_link_libraries(PiHex PRIVATE ws2_32)
#target_compile_options(PiHex PRIVATE "/GS-")
target_link_options(PiHex PRIVATE "/SAFESEH:NO")
# Include directories
target_include_directories(PiHex PRIVATE  ${CMAKE_SOURCE_DI}/)