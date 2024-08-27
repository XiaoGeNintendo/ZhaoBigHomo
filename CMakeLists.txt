cmake_minimum_required(VERSION 3.28)
project(ZhaoBigHomo)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG_MODE)
endif()

add_executable(fake_asm Lexer.h
        Lexer.cpp
        FakeAssembly.cpp
        FakeAssembly.h
        FakeAssemblyBuilder.h)
add_executable(real_product Lexer.h
        Lexer.cpp
        ZCompiler.cpp
        FakeAssembly.h
        FakeAssemblyBuilder.h
        Expression.cpp
        Expression.h
        CompilerTypes.h
)
