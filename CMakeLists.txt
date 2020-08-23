project(compiler)

set(CMAKE_CXX_STANDARD 17)

add_executable(compiler main.cpp AST.h AST.cpp Tokenizer.cpp Tokenizer.h Utils.h Utils.cpp AsmHelper.h AsmHelper.cpp Assembly.h Assembly.cpp Symbol.h Symbol.cpp RegControl.h RegControl.cpp)