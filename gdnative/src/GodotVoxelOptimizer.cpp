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

#include <File.hpp>
#include <GodotVoxelOptimizer.hpp>
#include <Image.hpp>
#include <GodotFileStream.hpp>
#include <Animation.hpp>
#include <AnimationPlayer.hpp>

void CGodotVoxelOptimizer::_register_methods()
{
    register_method("load", &CGodotVoxelOptimizer::Load);
    register_method("save", &CGodotVoxelOptimizer::Save);
    register_method("save_slices", &CGodotVoxelOptimizer::SaveSlices);
    register_method("get_meshes", &CGodotVoxelOptimizer::GetMeshes);
    register_method("get_statistics", &CGodotVoxelOptimizer::GetStatistics);
}

godot_error CGodotVoxelOptimizer::Load(String Path)
{
    Ref<File> file = File::_new();
    if(!file->file_exists(Path))
    {
        ERR_PRINT("File " + Path + " doesn't exists!");
        return (godot_error)Error::ERR_FILE_NOT_FOUND;
    }

    // Error err = file->open(Path, File::READ);
    // if(!file->is_open())
    // {
    //     ERR_PRINT("Couldn't open file: " + Path);
    //     return (godot_error)Error::ERR_FILE_CANT_READ;
    // }

    // if(err != Error::OK)
    //     return (godot_error)err;

    // PoolByteArray data = file->get_buffer(file->get_len());

    // m_Loader = VCore::IVoxelFormat::Create(VCore::IVoxelFormat::GetType(Path.utf8().get_data()));
    
    try
    {
        // m_Loader->Load((char*)data.read().ptr(), data.size());
        m_Loader = VCore::IVoxelFormat::CreateAndLoad<CGodotIOHandler>(Path.utf8().get_data());
    }
    catch(const VCore::CVoxelLoaderException &e)
    {
        file->close();

        ERR_PRINT(e.what());
        return (godot_error)Error::ERR_PARSE_ERROR;
    }
    
    // file->close();

    return (godot_error)Error::OK;
}

godot_error CGodotVoxelOptimizer::Save(String Path, bool exportWorldspace)
{
    if(m_Meshes.empty())
    {
        ERR_PRINT("No mesh data to save.");
        return (godot_error)Error::ERR_INVALID_DATA;
    }

    Ref<File> file = File::_new();
    VCore::Exporter exporter;

    exporter = VCore::IExporter::Create(VCore::IExporter::GetType(Path.utf8().get_data()));
    exporter->Settings()->WorldSpace = exportWorldspace;

    // Path = Path.get_basename();
    exporter->Save<CGodotIOHandler>(Path.utf8().get_data(), m_Meshes);
    
    return (godot_error)Error::OK;
}

godot_error CGodotVoxelOptimizer::SaveSlices(String Path)
{
    if(m_Meshes.empty())
    {
        ERR_PRINT("No mesh data to save.");
        return (godot_error)Error::ERR_INVALID_DATA;
    }

    Ref<File> file = File::_new();
    VCore::CSpriteStackingExporter exporter;

    auto models = m_Loader->GetModels();
    int count = 0;

    for (auto &&m : models)
    {
        auto image = exporter.Generate(m);

        if(models.size() > 1)
        {
            Path = Path.get_basename() + "_" + String::num(count) + "." + Path.get_extension();
            count++;
        }

        file->open(Path, File::WRITE);
        if(!file->is_open())
        {
            ERR_PRINT("Couldn't open file: " + Path);
            return (godot_error)Error::ERR_FILE_CANT_WRITE;
        }

        PoolByteArray data;
        data.resize(image.size());
        auto writer = data.write();
        memcpy(writer.ptr(), image.data(), image.size());

        file->store_buffer(data);
        file->close();
    }
    
    return (godot_error)Error::OK;
}

Array CGodotVoxelOptimizer::GetMeshes(int mesherType)
{
    Array ret;
    if(m_Loader->GetModels().empty())
    {
        ERR_PRINT("No file loaded please call load before!");
        return ret;
    }

    m_BlockCount = 0;
    m_VerticesCount = 0;
    m_FacesCount = 0;

    VCore::Mesher mesher = VCore::IMesher::Create((VCore::MesherTypes)mesherType);
    auto voxelmeshes = m_Loader->GetModels();
    for (auto &&m : voxelmeshes)
        m_BlockCount += m->GetBlockCount();
    
    m_Meshes = mesher->GenerateScene(m_Loader->GetSceneTree());
    // for (auto &&mesh : m_Meshes)
    for (size_t i = 0; i < m_Meshes.size(); i++)
    {
        auto mesh = m_Meshes[i];
        if(mesh->FrameTime == 0)
            ret.append(CreateMesh(mesh, m_Meshes.size() > 1));
        else
        {
            auto root = Spatial::_new();
            Ref<Animation> animation = Animation::_new();
            float length = 0;
            bool firstOne = true;
            for (; i < m_Meshes.size(); i++)
            {
                if(mesh->FrameTime == 0)
                {
                    i--;
                    break;
                }

                mesh = m_Meshes[i];
                auto gmesh = CreateMesh(mesh, m_Meshes.size() > 1);
                root->add_child(gmesh);
                gmesh->set_visible(firstOne);

                int64_t track_id = animation->add_track(Animation::TYPE_VALUE);
				animation->track_set_path(track_id, "./" + gmesh->get_name() + ":visible");
				animation->track_insert_key(track_id, length / 1000.0, true);
				animation->track_insert_key(track_id, (length + mesh->FrameTime) / 1000.0, false);
				
				length += mesh->FrameTime;
                firstOne = false;
            }

            auto player = AnimationPlayer::_new();
            root->add_child(player);
			animation->set_length(length / 1000.0);
			animation->set_loop(true);

			player->add_animation("Animation", animation);
			player->play("Animation");

            ret.append(root);
        }
    }
    
    // TODO: Find a good way to support animations in the ui.
    // auto animations = m_Loader->GetAnimations();
    // for (auto &&anim : animations)
    // {
    //     auto frames = mesher->GenerateAnimation(anim);
    //     Array animFrames;
    //     for (auto &&frame : frames)
    //     {
    //         Dictionary result;
    //         result["frameTime"] = frame.FrameTime;
    //         auto it = std::find(m_Meshes.begin(), m_Meshes.end(), frame.mesh);
    //         result["mesh"] = ret[it - m_Meshes.begin()];

    //         // result["mesh"] = CreateMesh(frame.mesh, meshes.size() > 1);
    //         animFrames.append(result);
    //     }

    //     ret.append(animFrames);
    // }

    return ret;
}

MeshInstance *CGodotVoxelOptimizer::CreateMesh(const VCore::Mesh &_Mesh, bool _Worldspace)
{
    Ref<ArrayMesh> tmpMesh = ArrayMesh::_new();
    Array arr;
    arr.resize(ArrayMesh::ARRAY_MAX);

    int surfaceIdx = 0;

    for (auto &&surface : _Mesh->Surfaces)
    {
        m_VerticesCount += surface.Size();
        m_FacesCount += surface.Indices.size() / 3;

        PoolIntArray indices;
        indices.resize(surface.Indices.size());
        memcpy(indices.write().ptr(), surface.Indices.data(), indices.size() * sizeof(int));  

        arr[ArrayMesh::ARRAY_VERTEX] = surface.Vertices;
        arr[ArrayMesh::ARRAY_NORMAL] = surface.Normals;
        arr[ArrayMesh::ARRAY_TEX_UV] = surface.UVs;
        arr[ArrayMesh::ARRAY_INDEX] = indices;
        tmpMesh->add_surface_from_arrays(ArrayMesh::PRIMITIVE_TRIANGLES, arr);

        tmpMesh->surface_set_material(surfaceIdx, ConvertMaterialToGodot(surface.FaceMaterial, _Mesh->Textures));
        surfaceIdx++;
    }

    auto instance = MeshInstance::_new();
    instance->set_mesh(tmpMesh);
    instance->set_flag(GeometryInstance::Flags::FLAG_USE_BAKED_LIGHT, true);

    // if(!_Mesh->Name.empty())
    //     instance->set_name(_Mesh->Name.c_str());

    // Worldspace
    if(_Worldspace)
    {
        auto matrix = _Mesh->ModelMatrix;
        instance->set_transform(Transform(matrix.x.x, matrix.y.x, matrix.z.x,
                                            matrix.x.y, matrix.y.y, matrix.z.y,
                                            matrix.x.z, matrix.y.z, matrix.z.z,
                                            matrix.x.w, matrix.y.w, matrix.z.w));
    }

    return instance;
}

Dictionary CGodotVoxelOptimizer::GetStatistics()
{
    Dictionary Ret;

    Ret["blocks"] = m_BlockCount;

    if(!m_Meshes.empty())
    {
        Ret["vertices"] = m_VerticesCount;
        Ret["faces"] = m_FacesCount;
    }
    else
    {
        Ret["vertices"] = 0;
        Ret["faces"] = 0;
    }

    return Ret;
}

Ref<ImageTexture> CGodotVoxelOptimizer::ConvertTextureToGodot(const VCore::Texture &_Texture)
{
    // Converts the colorpalette to a texture for godot.
    Ref<Image> img = Image::_new();

    img->create(_Texture->GetSize().x, _Texture->GetSize().y, false, Image::Format::FORMAT_RGBA8);
    img->lock();
    for (size_t x = 0; x < _Texture->GetSize().x; x++)
    {
        for (size_t y = 0; y < _Texture->GetSize().y; y++)
        {
            VCore::CColor c;
            c.FromRGBA(_Texture->GetPixel(VCore::Math::Vec2ui(x, y)));

            img->set_pixel(x, y, Color(c.R / 255.f, c.G / 255.f, c.B / 255.f, c.A / 255.f));
        }
    }
    img->unlock();

    Ref<ImageTexture> tex = ImageTexture::_new();
    tex->create_from_image(img, 0);

    return tex;
}

Ref<SpatialMaterial> CGodotVoxelOptimizer::ConvertMaterialToGodot(const VCore::Material &_Material, const std::map<VCore::TextureType, VCore::Texture> &_Textures)
{
    Ref<SpatialMaterial> material = SpatialMaterial::_new();

    material->set_texture(SpatialMaterial::TextureParam::TEXTURE_ALBEDO, ConvertTextureToGodot(_Textures.at(VCore::TextureType::DIFFIUSE)));
    material->set_metallic(_Material->Metallic);
    material->set_specular(_Material->Specular);
    material->set_roughness(_Material->Roughness);
    material->set_cull_mode(SpatialMaterial::CULL_FRONT);
    material->set_uv1_scale(Vector3(1, -1, 1));
    material->set_uv1_offset(Vector3(0, 1, 0));

    // if(_Material->IOR != 0.0)
    // {
    //     Mat->set_feature(SpatialMaterial::Feature::FEATURE_REFRACTION, true);
    //     Mat->set_refraction(_Material->IOR);
    // }        

    if(_Material->Transparency != 0.0)
    {
        material->set_feature(SpatialMaterial::Feature::FEATURE_TRANSPARENT, true);
        material->set_albedo(Color(1, 1, 1, 1 - _Material->Transparency));
    }

    if(_Material->Power != 0.0)
    {
        material->set_feature(SpatialMaterial::Feature::FEATURE_EMISSION, true);
        material->set_emission_energy(_Material->Power);

        material->set_texture(SpatialMaterial::TextureParam::TEXTURE_EMISSION, ConvertTextureToGodot(_Textures.at(VCore::TextureType::EMISSION)));
    }

    return material;
}