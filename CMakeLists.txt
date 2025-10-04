cmake_minimum_required(VERSION 3.18)
project(PiHex)

enable_language(C ASM_MASM)

# Set output directories
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add source files
set(SOURCES
    BLACK.ICO
    PIHEX.ICO
    CPUID.H
    CPUID.asm
    MAIN.H
    MAIN.C
    RESOURCES.h
    MAIN.RC
    PIHEX.C
    PIHEX.H
    POLYLOG.asm
    Polylogp5.asm
    Polylogp6.asm
)

# Add executable
add_executable(PiHex WIN32 ${SOURCES})
#set_target_properties(PiHex PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(PiHex PRIVATE ws2_32)
target_compile_options(PiHex PRIVATE "/GS-")
target_link_options(PiHex PRIVATE "/SAFESEH:NO")
# Include directories
target_include_directories(PiHex PRIVATE  ${CMAKE_SOURCE_DI}/)