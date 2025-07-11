cmake_minimum_required(VERSION 3.3.0)
project(vcore-tests VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS 
    "${PROJECT_SOURCE_DIR}/main.cpp"
    "${PROJECT_SOURCE_DIR}/TestCases_MaterialManager.cpp"
    "${PROJECT_SOURCE_DIR}/TestCases_VoxelSpace.cpp"
    "${PROJECT_SOURCE_DIR}/TestCases_Chunk.cpp"
    "${PROJECT_SOURCE_DIR}/TestCases_MagicaVoxelFormat.cpp"
    "${PROJECT_SOURCE_DIR}/TestCases_Culling.cpp"
    # "${PROJECT_SOURCE_DIR}/TestFileGenerator.cpp"
    )

include_directories("${PROJECT_SOURCE_DIR}/../lib/include")
add_subdirectory("${PROJECT_SOURCE_DIR}/../lib" VCore/)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} VCore)
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-static -static-libgcc -static-libstdc++>
)