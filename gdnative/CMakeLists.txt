cmake_minimum_required(VERSION 3.3.0)
project(godotVCore VERSION 0.1.0 LANGUAGES CXX C)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../lib libVCore)

set(SRCS "${PROJECT_SOURCE_DIR}/main.cpp"
         "${PROJECT_SOURCE_DIR}/src/GodotVCore.cpp")
set(LIBS "VCore")

if(WIN32)
    set(LIBS ${LIBS} "godot-cpp.windows.release.64")
else(WIND32)
    set(LIBS ${LIBS} "godot-cpp.linux.release.64")
endif(WIN32)


include_directories("${PROJECT_SOURCE_DIR}/../godot-cpp/include"
                    "${PROJECT_SOURCE_DIR}/../godot-cpp/include/core"
                    "${PROJECT_SOURCE_DIR}/../godot-cpp/include/gen"
                    "${PROJECT_SOURCE_DIR}/../godot-cpp/godot-headers"
                    "${PROJECT_SOURCE_DIR}/../lib/include"
                    "${PROJECT_SOURCE_DIR}/include")

link_directories("${PROJECT_SOURCE_DIR}/../godot-cpp/bin")


add_library(${PROJECT_NAME} SHARED ${SRCS})
target_link_libraries(${PROJECT_NAME} ${LIBS})