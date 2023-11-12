cmake_minimum_required(VERSION 3.3.0)
project(VCore VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(SRCS "${PROJECT_SOURCE_DIR}/main.cpp")
#set(LIBS "")

include_directories("${PROJECT_SOURCE_DIR}/Test")

add_subdirectory("${PROJECT_SOURCE_DIR}/lib")

install(FILES "${PROJECT_SOURCE_DIR}/lib/Test/VCore/VConfig.hpp" DESTINATION include)

# add_executable(${PROJECT_NAME} ${SRCS})
# target_link_libraries(${PROJECT_NAME} VCore)