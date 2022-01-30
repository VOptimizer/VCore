cmake_minimum_required(VERSION 3.3.0)
project(voxeloptimizer VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRCS "${PROJECT_SOURCE_DIR}/src/Loaders/VoxelMesh.cpp"

         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/MagicaVoxelFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/GoxelFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/KenshapeFormat.cpp"

         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleBinaryFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleBinaryTreeFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleExchangeFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleFormat.cpp"

         "${PROJECT_SOURCE_DIR}/src/Formats/IVoxelFormat.cpp"
         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/VEditFormat.cpp"

         "${PROJECT_SOURCE_DIR}/src/Meshers/SimpleMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshers/GreedyMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshers/IMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshers/MarchingCubesMesher.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshers/Slicer/Slicer.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshers/Slicer/BetterSlicer.cpp"
         "${PROJECT_SOURCE_DIR}/src/Meshers/MeshBuilder.cpp"
        #  "${PROJECT_SOURCE_DIR}/src/Meshers/VerticesReducer.cpp"
         "${PROJECT_SOURCE_DIR}/src/Exporters/WavefrontObjExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Exporters/IExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Exporters/SpriteStackingExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Exporters/GodotSceneExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Exporters/glTF/GLTFExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/Exporters/PLYExporter.cpp"
         "${PROJECT_SOURCE_DIR}/src/stb_image_write.c"
         "${PROJECT_SOURCE_DIR}/src/Texture.cpp")
#set(LIBS "")

include_directories("${PROJECT_SOURCE_DIR}/include"
                    "${PROJECT_SOURCE_DIR}/third_party")

add_library(${PROJECT_NAME} STATIC ${SRCS})
#target_link_libraries(${PROJECT_NAME} ${LIBS})