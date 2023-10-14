/*
 * MIT License
 *
 * Copyright (c) 2022 Christian Tost
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

#ifndef MESHCACHE_HPP
#define MESHCACHE_HPP

#include <algorithm>
#include <ArrayMesh.hpp>
#include <Image.hpp>
#include <Helpers/IterateableDictionary.hpp>
#include <Godot.hpp>
#include <Reference.hpp>
#include <String.hpp>
#include <VCore/VCore.hpp>

using namespace godot;

class CMeshCache : public Reference
{
    GODOT_CLASS(CMeshCache, Reference);
    public:
        CMeshCache() = default;

        void _init() { }

        static void _register_methods()
        {
            register_property<CMeshCache, Dictionary>("chunks", nullptr, &CMeshCache::GetMeshChunks, Variant());
            register_property<CMeshCache, String>("name", &CMeshCache::SetName, &CMeshCache::GetName, Variant());
            register_property<CMeshCache, int>("id", nullptr, &CMeshCache::GetID, Variant());
            
            register_signal<CMeshCache>("chunk_updated", "chunk_pos", GODOT_VARIANT_TYPE_VECTOR3, "mesh", GODOT_VARIANT_TYPE_OBJECT);
            register_signal<CMeshCache>("cleared");
        }

        inline Dictionary GetMeshChunks() const
        {
            return m_GodotMeshChunks;
        }

        inline void UpdateChunks(Dictionary _Chunks)
        {
            if(_Chunks.empty())
            {
                m_GodotMeshChunks.clear();
                emit_signal("cleared");
            }
            else
            {
                auto chunks = CIterateableDictionary(_Chunks);
                for (auto &&chunk : chunks)
                {
                    m_GodotMeshChunks[chunk.first] = chunk.second;
                    emit_signal("chunk_updated", chunk.first, chunk.second);
                }
            }
        }

        inline int GetID() const
        {
            return m_ID;
        }

        inline void SetID(int id)
        {
            m_ID = id;
        }

        inline String GetName() const
        {
            return String(m_Mesh->GetName().c_str());
        }

        inline void SetName(String name)
        {
            m_Mesh->SetName(name.utf8().get_data());
        }

        inline void SetVoxelMesh(VCore::VoxelMesh mesh)
        {
            m_Mesh = mesh;
        }

        inline VCore::VoxelMesh GetVoxelMesh() const
        {
            return m_Mesh;
        }

        ~CMeshCache() = default;
    private:
        int m_ID;
        VCore::VoxelMesh m_Mesh;
        Dictionary m_GodotMeshChunks;
        // Ref<ArrayMesh> m_GodotMesh;
};

#endif //MESHCACHE_HPP