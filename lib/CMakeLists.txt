cmake_minimum_required(VERSION 3.5.0)
project(VCore VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRCS "${PROJECT_SOURCE_DIR}/src/Voxel/VoxelModel.cpp"
         "${PROJECT_SOURCE_DIR}/src/Voxel/VoxelAnimation.cpp"
         "${PROJECT_SOURCE_DIR}/src/Voxel/PlanesVoxelizer.cpp"
         "${PROJECT_SOURCE_DIR}/src/Voxel/VoxelSpace.cpp"
         "${PROJECT_SOURCE_DIR}/src/Voxel/VoxelTextureMap.cpp"

         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/MagicaVoxelFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/GoxelFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/KenshapeFormat.cpp"

         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleBinaryFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleBinaryTreeFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleExchangeFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/IVoxelFormat.cpp"

         "${PROJECT_SOURCE_DIR}/src/Meshing/Implementations/SimpleMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshing/Implementations/GreedyMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshing/IMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshing/Implementations/Slicer/Slices.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshing/MeshBuilder.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshing/FaceMask.cpp"
         "${PROJECT_SOURCE_DIR}/src/Export/IExporter.cpp"

         "${PROJECT_SOURCE_DIR}/src/Misc/FileStream.cpp"
         "${PROJECT_SOURCE_DIR}/src/Misc/TexturePacker.cpp"

         "${PROJECT_SOURCE_DIR}/src/Export/Implementations/WavefrontObjExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Export/Implementations/SpriteStackingExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Export/Implementations/GodotSceneExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Export/Implementations/glTF/GLTFExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Export/Implementations/PLYExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Export/Implementations/fbx/FbxExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/stb_image_write.c"
         "${PROJECT_SOURCE_DIR}/src/Texture.cpp")
#set(LIBS "")

include_directories(
  "${PROJECT_SOURCE_DIR}/third_party"
  "${PROJECT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wunused-parameter -Wmismatched-tags -Wpessimizing-move>
)

INSTALL(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")

install(TARGETS ${PROJECT_NAME}
    # for executables and dll on Win
    RUNTIME DESTINATION bin
    # shared libraries
    LIBRARY DESTINATION lib
    # for static libraries
    ARCHIVE DESTINATION lib
    # public headers
    INCLUDES DESTINATION include)