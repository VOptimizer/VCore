cmake_minimum_required(VERSION 3.5.0)
project(cli VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS "${PROJECT_SOURCE_DIR}/main.cpp")
#set(LIBS "")

include_directories("${PROJECT_SOURCE_DIR}/../lib/include")

add_subdirectory("${PROJECT_SOURCE_DIR}/../lib" VCore/)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} VCore)
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-static -static-libgcc -static-libstdc++>
)