/*
 * MIT License
 *
 * Copyright (c) 2021 Christian Tost
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

#include <algorithm>
#include "argh.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <vector>
#include <VCore/VCore.hpp>
#include <chrono>

// #include <schrono>

using namespace std;
namespace fs = std::filesystem;

const vector<string> SUPPORTED_EXTS({"gox", "vox", "kenshape", "qbcl", "qb", "qbt", "qef"});
const vector<string> SUPPORTED_OUT_EXTS({"gltf", "glb", "obj", "escn2", "escn3", "ply", "png", "fbx"});

struct SFile
{
    string InputFile;
    string OutputFile;

    VCore::VoxelFormatType Type;
    VCore::ExporterType OutType;
    bool IsPNG;
};
using File = shared_ptr<SFile>;

void HelpDialog(const argh::parser &p_Cmdl)
{
    fs::path CliPath = p_Cmdl(0).str();
    string CliName = "./" + CliPath.filename().string();

    cout << "Usage: " << CliName << " [INPUT] [OPTIONS]\n" << endl;
    cout << "-h, --help\tThis dialog" << endl;
    cout << "-m, --mesher\tSets the mesher to meshify the voxel mesh. Default: simple. (simple, greedy, greedy_chunked, greedy_textured, smooth)" << endl;
    cout << "-o, --output\tOutput path. If the output path doesn't exist it will be created" << endl;
    cout << "-w, --worldspace\tTransforms all vertices to worldspace\n" << endl;
    cout << "--convert\tConverts a voxel model from one format to another one.\n" << endl;
    cout << "Examples:" << endl;
    cout << CliName << " windmill.vox -o windmill.glb\tConverts the *.vox file to a *.glb" << endl;
    cout << CliName << " voxels/*.vox -o *.glb\tConverts all *.vox files to *.glb with the same name as the *.vox files" << endl;
    cout << CliName << " voxels/*.vox -o output/Mesh{0}.glb\tConverts all *.vox files to a *.glb with the names Mesh0.glb Mesh1.glb ..." << endl;
    cout << CliName << " voxels/ -o *.glb\tConverts all supported file formats inside a folder to *.glb files" << endl;
    cout << CliName << " *.* -o *.glb\tConverts all supported file formats to *.glb files" << endl;
}

string ToLower(const string &p_Str)
{
    string Ret;

    for (auto &&c : p_Str)
        Ret += tolower(c);  

    return Ret;
}

File CreateFile(const fs::path &p_Input, const fs::path &p_OutputPattern)
{
    static size_t ID = 0;
    static map<string, VCore::VoxelFormatType> TYPE_MATCHER = {
        {"gox", VCore::VoxelFormatType::GOXEL},
        {"vox", VCore::VoxelFormatType::MAGICAVOXEL},
        {"kenshape", VCore::VoxelFormatType::KENSHAPE},
        {"qbcl", VCore::VoxelFormatType::QUBICLE},
        {"qb", VCore::VoxelFormatType::QUBICLE_BIN},
        {"qbt", VCore::VoxelFormatType::QUBICLE_BIN_TREE},
        {"qef", VCore::VoxelFormatType::QUBICLE_EXCHANGE},
    };

    static map<string, VCore::ExporterType> OUT_TYPE_MATCHER = {
        {"gltf", VCore::ExporterType::GLTF},
        {"glb", VCore::ExporterType::GLB},
        {"obj", VCore::ExporterType::OBJ},
        {"escn2", VCore::ExporterType::ESCN2},
        {"escn3", VCore::ExporterType::ESCN3},
        {"ply", VCore::ExporterType::PLY},
        {"fbx", VCore::ExporterType::FBX}
        // {"png", VCore::ExporterType::PNG},
    };

    File ret = File(new SFile());
    ret->IsPNG = false;

    ret->InputFile = p_Input.string();
    if(!p_OutputPattern.has_extension())
    {
        cerr << "Missing file extension: " << p_OutputPattern << endl;
        exit(-1);
    }

    string filename = p_OutputPattern.stem().string();
    string ext = p_OutputPattern.extension().string().substr(1);

    if(ToLower(ext) == "png")
        ret->IsPNG = true;
    else if(std::find(SUPPORTED_OUT_EXTS.begin(), SUPPORTED_OUT_EXTS.end(), ToLower(ext)) == SUPPORTED_OUT_EXTS.end())
    {
        cerr << "Unsupported file format: " << ext << endl;
        exit(-1);
    }

    if(filename.find("*") != string::npos)
        filename = regex_replace(filename, regex("\\*"), p_Input.stem().string());
    if(filename.find("{0}") != string::npos)
        filename = regex_replace(filename, regex("\\{0\\}"), to_string(ID++));

    if(p_OutputPattern.has_parent_path())
        ret->OutputFile = p_OutputPattern.parent_path().string() + "/";

    ret->OutputFile += filename + "." + ext;
    ret->Type = TYPE_MATCHER[ToLower(p_Input.extension().string().substr(1))];
    ret->OutType = OUT_TYPE_MATCHER[ToLower(ext)];

    return ret;
}

vector<File> ResolveFilenames(const argh::parser &p_Cmdl, const string &p_OutputPattern)
{
    vector<File> ret;
    fs::path outputPatternPath = p_OutputPattern;

    for (size_t i = 1; i < p_Cmdl.size(); i++)
    {
        fs::path inputPattern = p_Cmdl(i).str();
        if(inputPattern.filename().string().empty())
        {
            if(!fs::is_directory(inputPattern))
            {
                cerr << "Unsupported format: " << inputPattern << endl;
                exit(-1);
            }

            for(auto& p: fs::directory_iterator(inputPattern))
            {
                if(p.is_regular_file() && p.path().has_extension())
                {
                    auto it = std::find(SUPPORTED_EXTS.begin(), SUPPORTED_EXTS.end(), ToLower(p.path().extension().string().substr(1)));
                    if(it != SUPPORTED_EXTS.end())
                        ret.push_back(CreateFile(p.path(), outputPatternPath));
                }
            }

            continue;
        }

        if(!inputPattern.has_extension())
        {
            cerr << "Missing file extension: " << inputPattern << endl;
            exit(-1);
        }

        string filename = inputPattern.stem().string();
        string ext = inputPattern.extension().string().substr(1);

        if(ext == "*" || filename == "*")
        {
            vector<string> exts({ ext });

            // All supported formats
            if(ext == std::string("*"))
                exts = SUPPORTED_EXTS;
            else if(std::find(SUPPORTED_EXTS.begin(), SUPPORTED_EXTS.end(), ToLower(ext)) == SUPPORTED_EXTS.end())
            {
                cerr << "Unsupported file format: " << ext << endl;
                exit(-1);
            }

            for(auto& p: fs::directory_iterator(inputPattern.parent_path()))
            {
                if(p.is_regular_file() && p.path().has_extension())
                {
                    auto it = std::find(exts.begin(), exts.end(), ToLower(p.path().extension().string().substr(1)));
                    if(it != exts.end())
                    {
                        if(filename == "*" || ToLower(p.path().stem().string()) == ToLower(filename))
                            ret.push_back(CreateFile(p.path(), outputPatternPath));
                    }
                }
            }
        }
        else
        {
            if(std::find(SUPPORTED_EXTS.begin(), SUPPORTED_EXTS.end(), ext) == SUPPORTED_EXTS.end())
            {
                cerr << "Unsupported file format: " << ext << endl;
                exit(-1);
            }
            ret.push_back(CreateFile(inputPattern, outputPatternPath));
        }
    }

    return ret;
}

void GenerateMesh(const std::string &p_MesherType, const argh::parser &p_Cmdl, const std::string &p_OutputPattern)
{
    VCore::Mesher mesher;
    if(p_MesherType == "greedy")
        mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::GREEDY);
    else if(p_MesherType == "greedy_chunked")
        mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::GREEDY_CHUNKED);
    else if(p_MesherType == "greedy_textured")
        mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::GREEDY_TEXTURED);
    else if(p_MesherType == "smooth")
        mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::SMOOTH);
    else
        mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::SIMPLE);

    auto files = ResolveFilenames(p_Cmdl, p_OutputPattern);
    for (auto &&f : files)
    {
        VCore::VoxelFormat loader = VCore::IVoxelFormat::CreateAndOpen(f->InputFile, VCore::FileMode::READ);
        VCore::Exporter exporter;

        if(!f->IsPNG)
        {
            exporter = VCore::IExporter::Create(f->OutType);
            exporter->Settings->WorldSpace = p_Cmdl[{"-w", "--worldspace"}];
        }

        std::filesystem::path parent = fs::path(f->OutputFile).parent_path();
        if(!fs::is_directory(f->OutputFile) && !parent.empty())
            fs::create_directories(parent);

        auto loaderstartTime = std::chrono::high_resolution_clock::now();
        loader->Load();
        auto loaderendTime = std::chrono::high_resolution_clock::now();

        auto loaderduration = std::chrono::duration_cast<std::chrono::milliseconds>(loaderendTime - loaderstartTime);
        std::cout << "Loader time taken: " << loaderduration.count() << " ms" << std::endl;

        int counter = 0;

        if(f->IsPNG)
        {
            // auto meshes = loader->GetModels();
            // for (auto &&VoxelMesh : meshes)
            // {
            //     VCore::CSpriteStackingExporter stacker;
            //     std::string outputFilename = f->OutputFile;

            //     if(meshes.size() > 1)
            //     {
            //         fs::path outputFile = f->OutputFile;
            //         string Filename = outputFile.stem().string();
            //         string Ext = outputFile.extension().string().substr(1);

            //         outputFilename = outputFile.replace_filename(Filename + std::to_string(counter) + "." + Ext).string();
            //     }                    

            //     stacker.Save(outputFilename, VoxelMesh);
            //     counter++;
            // }
        }
        else
        {
            const int MAX_COUNT = 10;
            int64_t average = 0;

            // for (size_t i = 0; i < MAX_COUNT + 1; i++)
            // {
            //     auto startTime = std::chrono::high_resolution_clock::now();
            //     // auto meshes = Mesher->GenerateScene(Loader->GetSceneTree());
            //     (void)mesher->GenerateChunks(loader->GetModels()[0]);
            //     // Mesher->GenerateChunks(Loader->GetModels()[0]);
            //     auto endTime = std::chrono::high_resolution_clock::now();

            //     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            //     std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

            //     average += duration.count();
            // }

            // std::cout << "Average " << (average / (float)MAX_COUNT) << " ms" << std::endl;

            auto startTime = std::chrono::high_resolution_clock::now();
            auto renderTree = mesher->GenerateScene(loader->SceneTree);
            auto endTime = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

            exporter->Save(f->OutputFile, renderTree);
        }
    }
}

void Convert(const argh::parser &p_Cmdl)
{
    for (size_t i = 1; i < p_Cmdl.size(); i++)
    {
        fs::path inputPattern = p_Cmdl(i).str();
        VCore::VoxelFormat loader = VCore::IVoxelFormat::Create(VCore::VoxelFormatType::MAGICAVOXEL);
        loader->Open(p_Cmdl(i).str(), VCore::FileMode::STREAMED);
        loader->Load();

        // auto model = loader->GetModels()[0];

        // auto model = std::make_shared<VCore::CVoxelModel>();
        // for (size_t x = 0; x < 280; x += 256)
        // {
        //     for (size_t y = 0; y< 280; y += 256)
        //     {
        //         for (size_t z = 0; z< 280; z += 256)
        //         {
        //             model->SetVoxel(VCore::Math::Vec3i(x, y, z), 0, 0);
        //         }
        //     }
        // }

        // model->Name = "Test";
        
        // model->SetVoxel(VCore::Math::Vec3i(), 0, 1);




        VCore::VoxelFormat saver = VCore::IVoxelFormat::Create(VCore::VoxelFormatType::MAGICAVOXEL);
        saver->Open("convert.vox", VCore::FileMode::WRITE);
        // saver->m_Models = loader->GetModels(); //.push_back(model); //.push_back(model); //
        saver->SceneTree = loader->SceneTree;
        saver->Save();
    }
}

int main(int argc, char const *argv[])
{
    auto cmdl = argh::parser();
    cmdl.add_params({"-o", "--output", "-m", "--mesher", "--convert", "-w", "--worldspace"});
    cmdl.parse(argc, argv);

    // Shows the help dialog.
    if(cmdl[{"-h", "--help"}])
    {
        HelpDialog(cmdl);
        return 0;
    }

    std::string outputPattern;
    if(!(cmdl({"-o", "--output"}) >> outputPattern))
    {
        cerr << "Missing or wrong output format" << endl;
        HelpDialog(cmdl);
        return -1;
    }

    std::string mesherType;
    cmdl({"-m", "--mesher"}, "simple") >> mesherType;

    if(cmdl.size() == 1)
    {
        cerr << "Missing input files" << endl;
        return -1;
    }

    bool convert = false;
    convert = cmdl["--convert"];

    // VCore::Math::Vec3f a(1, 1, 1), b(1,1,1);
    // VCore::Math::Vec3i ai(1, 1, 1), bi(1,1,1);

    // float va[4] = {}, vb[4] = {};
    // memcpy(va, a.v, sizeof(float) * 3);
    // memcpy(vb, b.v, sizeof(float) * 3);

    // // std::cout << VCore::VectorEq(va, vb) << endl;

    // std::cout << (a == b) << endl;
    // std::cout << (ai == bi) << endl;

    try
    {
        if(!convert)
            GenerateMesh(mesherType, cmdl, outputPattern);
        else
            Convert(cmdl);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }

    return 0;
}
