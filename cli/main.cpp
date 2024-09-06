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
#include <filesystem>
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
const vector<string> SUPPORTED_OUT_EXTS({"gltf", "glb", "obj", "escn", "ply", "png", "fbx"});

struct SFile
{
    string InputFile;
    string OutputFile;

    VCore::LoaderType Type;
    VCore::ExporterType OutType;
    bool IsPNG;
};
using File = shared_ptr<SFile>;

void HelpDialog(const argh::parser &cmdl)
{
    fs::path CliPath = cmdl(0).str();
    string CliName = "./" + CliPath.filename().string();

    cout << "Usage: " << CliName << " [INPUT] [OPTIONS]\n" << endl;
    cout << "-h, --help\tThis dialog" << endl;
    cout << "-m, --mesher\tSets the mesher to meshify the voxel mesh. Default: simple. (simple, greedy, greedy_chunked, greedy_textured)" << endl;
    cout << "-o, --output\tOutput path. If the output path doesn't exist it will be created" << endl;
    cout << "-w, --worldspace\tTransforms all vertices to worldspace\n" << endl;
    cout << "Examples:" << endl;
    cout << CliName << " windmill.vox -o windmill.glb\tConverts the *.vox file to a *.glb" << endl;
    cout << CliName << " voxels/*.vox -o *.glb\tConverts all *.vox files to *.glb with the same name as the *.vox files" << endl;
    cout << CliName << " voxels/*.vox -o output/Mesh{0}.glb\tConverts all *.vox files to a *.glb with the names Mesh0.glb Mesh1.glb ..." << endl;
    cout << CliName << " voxels/ -o *.glb\tConverts all supported file formats inside a folder to *.glb files" << endl;
    cout << CliName << " *.* -o *.glb\tConverts all supported file formats to *.glb files" << endl;
}

string ToLower(const string &str)
{
    string Ret;

    for (auto &&c : str)
        Ret += tolower(c);  

    return Ret;
}

File CreateFile(const fs::path &Input, const fs::path &OutputPattern)
{
    static size_t ID = 0;
    static map<string, VCore::LoaderType> TYPE_MATCHER = {
        {"gox", VCore::LoaderType::GOXEL},
        {"vox", VCore::LoaderType::MAGICAVOXEL},
        {"kenshape", VCore::LoaderType::KENSHAPE},
        {"qbcl", VCore::LoaderType::QUBICLE},
        {"qb", VCore::LoaderType::QUBICLE_BIN},
        {"qbt", VCore::LoaderType::QUBICLE_BIN_TREE},
        {"qef", VCore::LoaderType::QUBICLE_EXCHANGE},
    };

    static map<string, VCore::ExporterType> OUT_TYPE_MATCHER = {
        {"gltf", VCore::ExporterType::GLTF},
        {"glb", VCore::ExporterType::GLB},
        {"obj", VCore::ExporterType::OBJ},
        {"escn", VCore::ExporterType::ESCN},
        {"ply", VCore::ExporterType::PLY},
        {"fbx", VCore::ExporterType::FBX}
        // {"png", VCore::ExporterType::PNG},
    };

    File Ret = File(new SFile());
    Ret->IsPNG = false;

    Ret->InputFile = Input.string();
    if(!OutputPattern.has_extension())
    {
        cerr << "Missing file extension: " << OutputPattern << endl;
        exit(-1);
    }

    string Filename = OutputPattern.stem().string();
    string Ext = OutputPattern.extension().string().substr(1);

    if(ToLower(Ext) == "png")
        Ret->IsPNG = true;
    else if(std::find(SUPPORTED_OUT_EXTS.begin(), SUPPORTED_OUT_EXTS.end(), ToLower(Ext)) == SUPPORTED_OUT_EXTS.end())
    {
        cerr << "Unsupported file format: " << Ext << endl;
        exit(-1);
    }

    if(Filename.find("*") != string::npos)
        Filename = regex_replace(Filename, regex("\\*"), Input.stem().string());
    if(Filename.find("{0}") != string::npos)
        Filename = regex_replace(Filename, regex("\\{0\\}"), to_string(ID++));

    if(OutputPattern.has_parent_path())
        Ret->OutputFile = OutputPattern.parent_path().string() + "/";

    Ret->OutputFile += Filename + "." + Ext;
    Ret->Type = TYPE_MATCHER[ToLower(Input.extension().string().substr(1))];
    Ret->OutType = OUT_TYPE_MATCHER[ToLower(Ext)];

    return Ret;
}

vector<File> ResolveFilenames(const argh::parser &cmdl, const string &OutputPattern)
{
    vector<File> Ret;
    fs::path OutputPatternPath = OutputPattern;

    for (size_t i = 1; i < cmdl.size(); i++)
    {
        fs::path InputPattern = cmdl(i).str();
        if(InputPattern.filename().string().empty())
        {
            if(!fs::is_directory(InputPattern))
            {
                cerr << "Unsupported format: " << InputPattern << endl;
                exit(-1);
            }

            for(auto& p: fs::directory_iterator(InputPattern))
            {
                if(p.is_regular_file() && p.path().has_extension())
                {
                    auto IT = std::find(SUPPORTED_EXTS.begin(), SUPPORTED_EXTS.end(), ToLower(p.path().extension().string().substr(1)));
                    if(IT != SUPPORTED_EXTS.end())
                        Ret.push_back(CreateFile(p.path(), OutputPatternPath));
                }
            }

            continue;
        }

        if(!InputPattern.has_extension())
        {
            cerr << "Missing file extension: " << InputPattern << endl;
            exit(-1);
        }

        string Filename = InputPattern.stem().string();
        string Ext = InputPattern.extension().string().substr(1);

        if(Ext == "*" || Filename == "*")
        {
            vector<string> Exts({ Ext });

            // All supported formats
            if(Ext == std::string("*"))
                Exts = SUPPORTED_EXTS;
            else if(std::find(SUPPORTED_EXTS.begin(), SUPPORTED_EXTS.end(), ToLower(Ext)) == SUPPORTED_EXTS.end())
            {
                cerr << "Unsupported file format: " << Ext << endl;
                exit(-1);
            }

            for(auto& p: fs::directory_iterator(InputPattern.parent_path()))
            {
                if(p.is_regular_file() && p.path().has_extension())
                {
                    auto IT = std::find(Exts.begin(), Exts.end(), ToLower(p.path().extension().string().substr(1)));
                    if(IT != Exts.end())
                    {
                        if(Filename == "*" || ToLower(p.path().stem().string()) == ToLower(Filename))
                            Ret.push_back(CreateFile(p.path(), OutputPatternPath));
                    }
                }
            }
        }
        else
        {
            if(std::find(SUPPORTED_EXTS.begin(), SUPPORTED_EXTS.end(), Ext) == SUPPORTED_EXTS.end())
            {
                cerr << "Unsupported file format: " << Ext << endl;
                exit(-1);
            }
            Ret.push_back(CreateFile(InputPattern, OutputPatternPath));
        }
    }

    return Ret;
}

int main(int argc, char const *argv[])
{
    auto cmdl = argh::parser();
    cmdl.add_params({"-o", "--output", "-m", "--mesher", "-b"});
    cmdl.parse(argc, argv);

    // Shows the help dialog.
    if(cmdl[{"-h", "--help"}])
    {
        HelpDialog(cmdl);
        return 0;
    }

    std::string OutputPattern;
    if(!(cmdl({"-o", "--output"}) >> OutputPattern))
    {
        cerr << "Missing or wrong output format" << endl;
        HelpDialog(cmdl);
        return -1;
    }

    std::string MesherType;
    cmdl({"-m", "--mesher"}, "simple") >> MesherType;

    if(cmdl.size() == 1)
    {
        cerr << "Missing input files" << endl;
        return -1;
    }

    int benchmarkCount = 0;
    cmdl({"-b"}, 0) >> benchmarkCount;

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
        VCore::Mesher Mesher;
        if(MesherType == "greedy")
            Mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::GREEDY);
        else if(MesherType == "greedy_chunked")
            Mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::GREEDY_CHUNKED);
        else if(MesherType == "greedy_textured")
            Mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::GREEDY_TEXTURED);
        else
            Mesher = VCore::IMesher::Create<VCore::DefaultSurface>(VCore::MesherTypes::SIMPLE);

        auto Files = ResolveFilenames(cmdl, OutputPattern);
        for (auto &&f : Files)
        {
            VCore::VoxelFormat Loader = VCore::IVoxelFormat::Create(f->Type);
            VCore::Exporter Exporter;

            if(!f->IsPNG)
            {
                Exporter = VCore::IExporter::Create(f->OutType);
                Exporter->Settings->WorldSpace = cmdl[{"-w", "--worldspace"}];
            }

            std::filesystem::path parent = fs::path(f->OutputFile).parent_path();
            if(!fs::is_directory(f->OutputFile) && !parent.empty())
                fs::create_directories(parent);

            Loader->Load(f->InputFile);
            int counter = 0;

            if(f->IsPNG)
            {
                auto meshes = Loader->GetModels();
                for (auto &&VoxelMesh : meshes)
                {
                    VCore::CSpriteStackingExporter Stacker;
                    std::string outputFilename = f->OutputFile;

                    if(meshes.size() > 1)
                    {
                        fs::path outputFile = f->OutputFile;
                        string Filename = outputFile.stem().string();
                        string Ext = outputFile.extension().string().substr(1);

                        outputFilename = outputFile.replace_filename(Filename + std::to_string(counter) + "." + Ext).string();
                    }                    

                    Stacker.Save(outputFilename, VoxelMesh);
                    counter++;
                }
            }
            else
            {
                std::vector<VCore::Mesh> outputMeshes;
                const int MAX_COUNT = 10;
                int64_t average = 0;

                for (size_t i = 0; i < MAX_COUNT + 1; i++)
                {
                    auto startTime = std::chrono::high_resolution_clock::now();
                    // auto meshes = Mesher->GenerateScene(Loader->GetSceneTree());
                    (void)Mesher->GenerateChunks(Loader->GetModels()[0]);
                    // Mesher->GenerateChunks(Loader->GetModels()[0]);
                    auto endTime = std::chrono::high_resolution_clock::now();

                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                    std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

                    average += duration.count();
                }

                std::cout << "Average " << (average / (float)MAX_COUNT) << " ms" << std::endl;

                auto meshes = Mesher->GenerateScene(Loader->GetSceneTree());
                outputMeshes.insert(outputMeshes.end(), meshes.begin(), meshes.end());
                Exporter->Save(f->OutputFile, outputMeshes);
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }

    return 0;
}
