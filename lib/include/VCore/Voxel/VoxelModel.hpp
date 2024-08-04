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
#include <VCore/Voxel/Voxel.hpp>
#include <VCore/Voxel/BBox.hpp>
#include <vector>
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Meshing/Material.hpp>
#include <VCore/Voxel/VoxelSpace.hpp>
#include <VCore/Meshing/Texture.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <VCore/Math/Vector.hpp>
#include <VCore/Voxel/VoxelTextureMap.hpp>

namespace VCore
{
    enum class TexturingTypes
    {
        INDEXED,    //!< Each ColorIdx of a voxel is an index for a single color.
        TEXTURED    //!< Each ColorIdx of a voxel is an index to a tile in a texture atlas.
    };

    class CVoxelModel
    {
        using VoxelData = CVoxelSpace;

        public:
            // For the gui
            std::string Name;

            // Texturing
            TexturingTypes TexturingType;
            CVoxelTextureMap TextureMapping;           //!< Contains information about the texture atlas mapping.

            std::vector<Material> Materials;           //!< Used materials
            std::map<TextureType, Texture> Textures;   //!< Used colors / texture atlas

            CVoxelModel() : TexturingType(TexturingTypes::INDEXED) {}
            
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
            void SetVoxel(const Math::Vec3i &Pos, int Material, int Color, bool Transparent);

            /**
             * @brief Removes a voxel on a given position
             * 
             * @param Pos: Position of the voxel to remove.
             */
            void RemoveVoxel(const Math::Vec3i &Pos);

            /**
             * @brief Clears all voxeldata.
             */
            void Clear();

            /**
             * @return Gets a voxel on a given position.
             */
            Voxel GetVoxel(const Math::Vec3i &Pos);

            /**
             * @return Returns the bbox of the voxel model.
             */
            CBBox GetBBox() const
            {
                return m_Voxels.calculateBBox();
            }

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
             * @brief Queries all visible voxels.
             * @param opaque: If true only opaque voxels are returned, otherwise only none opaque voxels are returned.
             */
            VectoriMap<Voxel> QueryVisible(bool opaque) const;

            /**
             * @return Gets a list of all chunks which has been modified.
             * @note Marks all chunks as processed.
             */
            VoxelData::querylist QueryDirtyChunks();

            /**
             * @return Returns all chunks.
             */
            VoxelData::querylist QueryChunks() const;

            /**
             * @return Returns all chunks inside of the frustum
             */
            VoxelData::querylist QueryChunks(const CFrustum *_Frustum) const;
            
            ~CVoxelModel() = default;
        private:             
            VoxelData m_Voxels;
    };

    using VoxelModel = std::shared_ptr<CVoxelModel>;
}


#endif //VOXELMESH_HPP