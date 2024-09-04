/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/**
 * Simple program demonstrating the meshing pipeline.
 */

#include <VCore/VCore.hpp>

using namespace VCore;

int main(int argc, char const *argv[])
{
    // Create the corresponding loader for the given voxel file and initiate loading.
    // This function utilizes the default file stream for file loading.
    // You can create your own derivation of the file stream for better integration
    // with your engine/framework.
    IVoxelFormat format = IVoxelFormat::CreateAndLoad("windmill.vox");

    // Create a new mesher instance of type Simple.
    // Use the default surface for meshing.
    IMesher mesher = IMesher::Create<DefaultSurface>(MesherTypes::SIMPLE);

    // Mesh all voxel models in the scene.
    std::vector<Mesh> meshes = mesher->GenerateScene(format->GetModels());

    // Create a new exporter of type GLB (GLTF binary format).
    IExporter exporter = IExporter::Create(ExporterType::GLB);

    // Export all meshes in the scene to the file 'windmill.glb'.
    exporter->Save("windmill.glb", meshes);

    return 0;
}