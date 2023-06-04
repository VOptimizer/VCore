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

#ifndef VOXELMESH_HPP
#define VOXELMESH_HPP

#include <array>
#include <VoxelOptimizer/Voxel/Voxel.hpp>
#include <VoxelOptimizer/Voxel/BBox.hpp>
#include <list>
#include <VoxelOptimizer/Math/Mat4x4.hpp>
#include <VoxelOptimizer/Meshing/Material.hpp>
#include <VoxelOptimizer/Voxel/VoxelSpace.hpp>
#include <VoxelOptimizer/Meshing/Texture.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    class CVoxelMesh
    {
        using VoxelData = CVoxelSpace;//COctree<Voxel>;

        public:
            // For the gui
            std::string Name;
            Texture Thumbnail;

            Math::Vec3f Pivot;
            CBBox BBox;

            std::vector<Material> Materials;                //!< Used materials
            std::map<TextureType, Texture> Colorpalettes;   //!< Used colors / textures

            CVoxelMesh() = default;

            /**
             * @brief Sets the size of the voxel space.
             */
            inline void SetSize(const Math::Vec3i &Size)
            {
                m_Size = Size;
                // m_Voxels.set_size(m_Size);
                // m_Pool.clear();
            }

            /**
             * The voxel space begins always at (0, 0, 0) which is the bottom, front left corner.
             * The z axis is the Up axis and y the forward axis.
             * 
             * @return Returns the voxel space size of a voxel mesh.
             */
            inline Math::Vec3i GetSize() const
            {
                return m_Size;
            }

            /**
             * @brief Gets the bounding box.
             */
            inline CBBox GetBBox() const
            {
                return BBox;
            }
            
            /**
             * @brief Sets the bounding box.
             */
            inline void SetBBox(CBBox BBox)
            {
                BBox = BBox;
            }

            inline void RecalcBBox()
            {
                BBox = CBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i(0, 0, 0));

                for (auto &&v : m_Voxels)
                {
                    BBox.Beg = BBox.Beg.min(v.first);
                    BBox.End = BBox.End.max(v.first + Math::Vec3i(1, 1, 1));
                }
            }
            
            /**
             * List of voxels. The size of the list is always the size of the voxel space.
             * Voxels which are null are empty space.
             * 
             * @return Returns the list of voxels.
             */
            inline VoxelData &GetVoxels() //const
            {
                return m_Voxels;
            }

            /**
             * @brief Sets a voxel with an given material index.
             * 
             * @param Pos: The position of the voxel inside the voxel space.
             * @param Material: Material index of the voxels material.
             * @param Color: Color index.
             * @param Transparent: Is the block transparent?
             */
            void SetVoxel(const Math::Vec3i &Pos, int Material, int Color, bool Transparent, CVoxel::Visibility mask = CVoxel::Visibility::VISIBLE);

            /**
             * @brief Removes a voxel on a given position
             * 
             * @param Pos: Position of the voxel to remove.
             */
            void RemoveVoxel(const Math::Vec3i &Pos);

            /**
             * @brief Clears the mesh
             */
            void Clear();

            void ReserveVoxels(size_t _reserve)
            {
                // m_Pool.reserve(_reserve);
            }

            /**
             * @return Gets a voxel on a given position.
             */
            Voxel GetVoxel(const Math::Vec3i &Pos);

            /**
             * @brief Gets a voxel on a given position.
             * 
             * @param pos: Position of the voxel
             * @param OpaqueOnly: If true than only opaque voxels are returned, otherwise all transparent one.
             */
            Voxel GetVoxel(const Math::Vec3i &Pos, bool OpaqueOnly);

            /**
             * @return Gets a visible voxel on a given position.
             */
            Voxel GetVisibleVoxel(const Math::Vec3i &Pos);

            /**
             * @brief Gets a visible voxel on a given position.
             * 
             * @param pos: Position of the voxel
             * @param OpaqueOnly: If true than only opaque voxels are returned, otherwise all transparent one.
             */
            Voxel GetVisibleVoxel(const Math::Vec3i &Pos, bool OpaqueOnly);

            /**
             * @brief Gets the count of all setted blocks.
             */
            inline size_t GetBlockCount() const
            {
                return m_Voxels.size();
            }

            /**
             * @brief Generates the visibility mask for each voxel.
             * @note Only dirty chunks are queried.
             */
            void GenerateVisibilityMask();

            /**
             * @brief Queries all visible voxels.
             * @param opaque: If true only opaque voxels are returned, otherwise only none opaque voxels are returned.
             */
            VectoriMap<Voxel> QueryVisible(bool opaque) const;

            /**
             * @return Gets a list of all chunks which has been modified.
             * @note Marks all chunks as processed.
             */
            std::list<SChunkMeta> QueryDirtyChunks();

            /**
             * @return Returns all chunks.
             */
            std::list<SChunkMeta> QueryChunks() const;
            
            ~CVoxelMesh() = default;
        private:   
            void SetNormal(const Math::Vec3i &Pos, const Math::Vec3i &Neighbor, bool IsInvisible = true);

            Math::Vec3i m_Size;            
            VoxelData m_Voxels;
    };

    using VoxelMesh = std::shared_ptr<CVoxelMesh>;
}


#endif //VOXELMESH_HPP