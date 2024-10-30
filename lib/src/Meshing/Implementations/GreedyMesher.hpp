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

#include <vector>
#include <VCore/Meshing/IMesher.hpp>
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include "../FaceMask.hpp"

#include <chrono>
#include <atomic>

namespace VCore
{
    class CGreedyMesher : public IMesher
    {
        public:
            CGreedyMesher(bool _GenerateTexture = false, bool _GenerateSingleChunks = false) : IMesher(), m_GenerateTexture(_GenerateTexture), m_GenerateSingleChunks(_GenerateSingleChunks), m_MaskgenerationTime(0), m_WidthgenerationTime(0) {}

            std::vector<SMeshChunk> GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty = false) override;

            virtual ~CGreedyMesher() = default;
        protected:
            using MaskCollection = ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>>;
            struct TaskContext
            {
                TaskContext(int _Axis, int _SliceProcessCount, const VoxelModel &_Model, const CBBox &_ModelBBox, int _NextSlice) : Axis(_Axis), SliceProcessCount(_SliceProcessCount), Model(_Model), ModelBBox(_ModelBBox), NextSlice(_NextSlice) {}

                const int Axis;
                const int SliceProcessCount;
                const VoxelModel &Model;
                const CBBox &ModelBBox;
                std::atomic<int> NextSlice;
            };
            
            struct MeshSlicerContext
            {
                MeshSlicerContext(const VoxelModel &_Model, const CBBox &_ModelBBox, SurfaceFactory _Factory) : Model(_Model), ModelBBox(_ModelBBox), Builder(_Factory) 
                {
                    Builder.AddTextures(Model->Textures);
                }   

                const VoxelModel &Model;
                CMeshBuilder Builder;
                const CBBox &ModelBBox;
                MaskCollection::iterator DepthIt;
                ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>::iterator SliceIt;
                Math::Vec3i Axis;
                Math::Vec3i Position;
                ankerl::unordered_dense::map<Math::Vec3i, MaskCollection, Math::Vec3iHasher> Chunks;
            };

            Mesh SlicerTask(TaskContext &_TaskContext);

            SMeshChunk GenerateMeshChunk(VoxelModel, const SChunkMeta&, bool) override;

            Mesh GenerateMeshSlices(const VoxelModel &_Model, const CBBox &_ModelBBox, int _RunAxis, int _AxisPos);
            void GenerateMeshSlice(MeshSlicerContext &_Context, BITMASK_TYPE _Faces, bool _IsFront);

            /**
             * Gets a column of faces for a given chunkpos. If the chunk isn't indexed,
             * a new chunkmask is generated and inserted into the Chunks map of the _Context object.
             * 
             * @param _Context: Current slicer context
             * @param _Chunkpos: Position of the chunk to process.
             * @param d: Current slice depth inside of the chunk, reanges from 0 - (CHUNK_SIZE - 1)
             * @param x: Current column position of the slice.
             * @param _IsFront: Tells which faces we are interested in, either backfaces (false) or frontfaces (true).
             * @param _Found: Will be true, if for the given _Chunkpos, d and x tripple a column of faces where found.
             * 
             * @return Returns the faces column.
             */
            BITMASK_TYPE *GetFaces(MeshSlicerContext &_Context, const Math::Vec3i &_Chunkpos, int d, int x, bool _IsFront);

            CFaceMask::Mask *GetFaceMask(MeshSlicerContext &_Context, const Math::Vec3i &_Chunkpos, int d);

            bool m_GenerateTexture;
            bool m_GenerateSingleChunks;

            std::chrono::milliseconds m_MaskgenerationTime, m_WidthgenerationTime;

            void GenerateQuad(CMeshBuilder &result, const std::vector<Material> &_Materials, BITMASK_TYPE faces, CFaceMask::Mask &bits, int width, int depth, bool isFront, const Math::Vec3i &axis, const SChunkMeta &_Chunk, const Voxel _Voxel);
    };
}

#endif