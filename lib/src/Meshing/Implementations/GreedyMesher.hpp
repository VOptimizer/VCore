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

#ifndef GREEDYMESHER_HPP
#define GREEDYMESHER_HPP

#include <VCore/Meshing/IMesher.hpp>
#include <VCore/Memory/ObjectPool.hpp>
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include "../FaceMask.hpp"

#include <atomic>
#include <cstdint>

namespace VCore
{
    class CGreedyMesher : public IMesher
    {
        public:
            CGreedyMesher(bool p_GenerateTexture = false, bool p_GenerateSingleChunks = false) : 
                IMesher(), 
                m_NextId(0),
                m_Head(nullptr),
                m_GenerateTexture(p_GenerateTexture), 
                m_GenerateSingleChunks(p_GenerateSingleChunks) {}

            // std::vector<SMeshChunk> GenerateChunks(VoxelModel p_Mesh, bool p_OnlyDirty = false) override;

            virtual ~CGreedyMesher() { ClearTextures(); }
        protected:
            using MaskCollection = ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint64_t, CFaceMask::Mask>>;            
            struct MeshSlicerContext
            {
                MeshSlicerContext(const VoxelModel &p_Model, const CBBox &p_ModelBBox, SurfaceFactory p_Factory) : Model(p_Model), Builder(p_Factory), ModelBBox(p_ModelBBox)
                {
                    // Builder.AddTextures(Model->Textures);
                }   

                const VoxelModel &Model;
                CMeshBuilder Builder;
                const CBBox &ModelBBox;
                MaskCollection::iterator DepthIt;
                ankerl::unordered_dense::map<uint64_t, CFaceMask::Mask>::iterator SliceIt;
                Math::Vec3i Axis;
                Math::Vec3i Position;
                ankerl::unordered_dense::map<Math::Vec3i, MaskCollection, Math::Vec3iHasher> Chunks;
            };

            struct TextureInfo
            {
                TextureInfo() = default;
                TextureInfo(uint32_t _Id, const Math::Vec3i &p_Position, const Math::Vec3i &p_Axis, const Math::Vec2ui &p_Size) : Id(_Id), Position(p_Position), Axis(p_Axis), Size(p_Size) {}
                TextureInfo(TextureInfo &&) = default;
                TextureInfo(const TextureInfo &) = default;

                TextureInfo &operator=(TextureInfo &&) = default;
                TextureInfo &operator=(const TextureInfo &) = default;

                uint32_t Id;
                Math::Vec3i Position;
                Math::Vec3i Axis;
                Math::Vec2ui Size;
            };

            struct TextureNode
            {
                TextureNode(uint32_t p_Id, const Math::Vec3i &p_Position, const Math::Vec3i &p_Axis, const Math::Vec2ui &p_Size) : Info(p_Id, p_Position, p_Axis, p_Size), Next(nullptr) {}

                TextureInfo Info;
                TextureNode *Next;
            };
            uint32_t AddTexture(const Math::Vec3i &p_Position, const Math::Vec3i &p_Axis, const Math::Vec2ui &p_Size);
            void ClearTextures();

            void CopyToAtlas(Texture &p_Atlas, const VoxelModel &p_Model, const Math::Vec2ui &p_Position, const TextureInfo &p_Info);

            std::atomic<uint32_t> m_NextId;
            std::atomic<TextureNode*> m_Head;

            Memory::CObjectPool<TextureNode> m_Pool;

            // uint32_t m_NextId;
            // TextureNode* m_Head;

            // std::mutex m_Lock;

            SMeshChunk GenerateMeshChunk(VoxelModel, const SChunkMeta&, bool) override;

            Mesh GenerateMeshSlices(const VoxelModel &p_Model, const CBBox &p_ModelBBox, int p_RunAxis, int p_AxisPos);
            void GenerateMeshSlice(MeshSlicerContext &p_Context, Config::bitmask_t p_Faces, bool p_IsFront);

            /**
             * Gets a column of faces for a given chunkpos. If the chunk isn't indexed,
             * a new chunkmask is generated and inserted into the Chunks map of the _Context object.
             * 
             * @param _Context: Current slicer context
             * @param _Chunkpos: Position of the chunk to process.
             * @param d: Current slice depth inside of the chunk, reanges from 0 - Config::InnerChunkMask
             * @param x: Current column position of the slice.
             * @param _IsFront: Tells which faces we are interested in, either backfaces (false) or frontfaces (true).
             * @param _Found: Will be true, if for the given _Chunkpos, d and x tripple a column of faces where found.
             * 
             * @return Returns the faces column.
             */
            Config::bitmask_t *GetFaces(MeshSlicerContext &p_Context, const Math::Vec3i &p_Chunkpos, int p_Depth, int p_XPos, bool p_IsFront);

            CFaceMask::Mask *GetFaceMask(MeshSlicerContext &p_Context, const Math::Vec3i &p_Chunkpos, int p_Depth);

            bool m_GenerateTexture;
            bool m_GenerateSingleChunks;

            void GenerateQuad(CMeshBuilder &p_Result, Config::bitmask_t p_Faces, CFaceMask::Mask &p_Bits, int p_Width, int p_Depth, bool p_IsFront, const Math::Vec3i &p_Origin, const Math::Vec3i &p_Axis, const SChunkMeta &p_Chunk, const CVoxel& p_Voxel, uint8_t p_Ao);
    };
}

#endif