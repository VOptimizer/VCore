cmake_minimum_required(VERSION 3.5.0)
project(VCore VERSION 0.1.0 LANGUAGES CXX C)

# Basic compiler setup
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
set(VCORE_BUILD_ALL_IMPORTERS_BY_DEFAULT ON CACHE BOOL "Builds all voxel format importes")
set(VCORE_BUILD_ALL_EXPORTERS_BY_DEFAULT ON CACHE BOOL "Builds all exporters")

set(CORE_SRCS "${PROJECT_SOURCE_DIR}/src/Voxel/VoxelAnimation.cpp"
             "${PROJECT_SOURCE_DIR}/src/Voxel/Storage/VoxelSpace.cpp"
             "${PROJECT_SOURCE_DIR}/src/Voxel/Storage/Iterator.cpp"
             "${PROJECT_SOURCE_DIR}/src/Voxel/Storage/Chunk.cpp"
             
             "${PROJECT_SOURCE_DIR}/src/Meshing/IMesher.cpp"
             "${PROJECT_SOURCE_DIR}/src/Meshing/Implementations/SimpleMesher.cpp"
             "${PROJECT_SOURCE_DIR}/src/Meshing/Implementations/GreedyMesher.cpp"

             "${PROJECT_SOURCE_DIR}/src/Meshing/MeshBuilder.cpp"
             "${PROJECT_SOURCE_DIR}/src/Meshing/FaceMask.cpp"
             "${PROJECT_SOURCE_DIR}/src/Meshing/MaterialManager.cpp"
             
             "${PROJECT_SOURCE_DIR}/src/Misc/FileStream.cpp"
             "${PROJECT_SOURCE_DIR}/src/Texture.cpp"
             
             "${PROJECT_SOURCE_DIR}/src/Formats/IVoxelFormat.cpp"
             "${PROJECT_SOURCE_DIR}/src/Export/IExporter.cpp"
             "${PROJECT_SOURCE_DIR}/src/stb_image_write.c")

set(IMPORTER_SRCS "")
set(EXPORTER_SRCS "")

# Helper macro to check if an importer should be build or not
macro(ADD_IMPORTER name)
  if(VCORE_BUILD_ALL_IMPORTERS_BY_DEFAULT)
    set(IMPORTER_ENABLED TRUE)
    if(DEFINED VCORE_BUILD_${name}_IMPORTER AND NOT VCORE_BUILD_${name}_IMPORTER)
      set(IMPORTER_ENABLED FALSE)
    endif()
  else()
    set(IMPORTER_ENABLED ${VCORE_BUILD_${name}_IMPORTER})
  endif()

  if(IMPORTER_ENABLED)
    list(APPEND IMPORTER_SRCS ${ARGN})
  else()
    add_definitions(-DVCORE_BUILD_NO_${name}_IMPORTER)
  endif()
endmacro(ADD_FEATURE)

ADD_IMPORTER(MAGICAVOXEL "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/MagicaVoxel/MagicaVoxelFormat.cpp"
                         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/MagicaVoxel/MagicaVoxelModelParser.cpp"
                         "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/MagicaVoxel/MagicaVoxelStreamable.cpp")

ADD_IMPORTER(GOXEL "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Goxel/GoxelFormat.cpp"
                   "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Goxel/GoxelModelParser.cpp"
                   "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Goxel/GoxelStreamable.cpp")
ADD_IMPORTER(KENSHAPE "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/KenshapeFormat.cpp")

ADD_IMPORTER(QB "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleBinaryFormat.cpp")  
ADD_IMPORTER(QBT "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleBinaryTreeFormat.cpp")
ADD_IMPORTER(QEF "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleExchangeFormat.cpp")
ADD_IMPORTER(QBCL "${PROJECT_SOURCE_DIR}/src/Formats/Implementations/Qubicle/QubicleFormat.cpp")

# Helper macro to check if an exporter should be build or not
macro(ADD_EXPORTER name)
  if(VCORE_BUILD_ALL_EXPORTERS_BY_DEFAULT)
    set(EXPORTER_ENABLED TRUE)
    if(DEFINED VCORE_BUILD_${name}_EXPORTER AND NOT VCORE_BUILD_${name}_EXPORTER)
      set(EXPORTER_ENABLED FALSE)
    endif()
  else()
    set(EXPORTER_ENABLED ${VCORE_BUILD_${name}_EXPORTER})
  endif()

  if(EXPORTER_ENABLED)
    list(APPEND EXPORTER_SRCS ${ARGN})
  else()
    add_definitions(-DVCORE_BUILD_NO_${name}_EXPORTER)
  endif()
endmacro(ADD_FEATURE)   

ADD_EXPORTER(OBJ "${PROJECT_SOURCE_DIR}/src/Export/Implementations/WavefrontObjExporter.cpp")
ADD_EXPORTER(SPRITESTACKING "${PROJECT_SOURCE_DIR}/src/Export/Implementations/SpriteStackingExporter.cpp")
ADD_EXPORTER(GODOT3 "${PROJECT_SOURCE_DIR}/src/Export/Implementations/GodotSceneExporter.cpp")
ADD_EXPORTER(GLTF "${PROJECT_SOURCE_DIR}/src/Export/Implementations/glTF/GLTFExporter.cpp")
ADD_EXPORTER(PLY "${PROJECT_SOURCE_DIR}/src/Export/Implementations/PLYExporter.cpp")
ADD_EXPORTER(FBX "${PROJECT_SOURCE_DIR}/src/Export/Implementations/fbx/FbxExporter.cpp")

include_directories(
  "${PROJECT_SOURCE_DIR}/third_party"
  "${PROJECT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME} STATIC ${CORE_SRCS} ${IMPORTER_SRCS} ${EXPORTER_SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wreturn-type -Wextra -Wunused-parameter -Wmismatched-tags -Wpessimizing-move -march=native>
)

find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_custom_target(cppcheck
      COMMAND ${CPPCHECK} --enable=all --std=c++${CMAKE_CXX_STANDARD} --output-file=log.sarif --output-format=sarif -q -I "${PROJECT_SOURCE_DIR}/third_party" -I "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/src"
  )
endif()

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