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
#include <VoxelOptimizer/Voxel/BBox.hpp>
#include <list>
#include <VoxelOptimizer/Math/Mat4x4.hpp>
#include <VoxelOptimizer/Meshing/Material.hpp>
#include <VoxelOptimizer/Voxel/Octree.hpp>
#include <VoxelOptimizer/Meshing/Texture.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <VoxelOptimizer/Memory/ObjectPool.hpp>
#include <string>
#include <vector>
#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    class CVoxel
    {
        public:
            enum Visibility : uint8_t
            {
                INVISIBLE = 0,
                UP = 1,
                DOWN = 2,
                LEFT = 4,
                RIGHT = 8,
                FORWARD = 16,
                BACKWARD = 32,

                VISIBLE = (UP | DOWN | LEFT | RIGHT | FORWARD | BACKWARD)
            };

            // Normal face directions inside the voxel space.
            const static CVector FACE_UP; // (0, 0, 1);
            const static CVector FACE_DOWN; // (0, 0, -1);
            const static CVector FACE_LEFT; // (-1, 0, 0);
            const static CVector FACE_RIGHT; // (1, 0, 0);
            const static CVector FACE_FORWARD; // (0, 1, 0);
            const static CVector FACE_BACKWARD; // (0, -1, 0);
            const static CVector FACE_ZERO; // (0, 0, 0);

            CVoxel();

            CVector Pos;
            int Material;   //!< Index of the material.
            int Color;      //!< Index of the color.
            bool Transparent;

            Visibility VisibilityMask;

            /**
             * @return Returns true if at leas one side is visible.
             */
            inline bool IsVisible() const
            {                
                return VisibilityMask != Visibility::INVISIBLE;
            }

            ~CVoxel() = default;
    };

    inline CVoxel::Visibility operator&(CVoxel::Visibility lhs, CVoxel::Visibility rhs)
    {
        return static_cast<CVoxel::Visibility>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

    inline CVoxel::Visibility operator&=(CVoxel::Visibility &lhs, const CVoxel::Visibility rhs)
    {
        lhs = static_cast<CVoxel::Visibility>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
        return lhs;
    }

    inline CVoxel::Visibility operator|=(CVoxel::Visibility &lhs, const CVoxel::Visibility rhs)
    {
        lhs = static_cast<CVoxel::Visibility>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
        return lhs;
    }

    inline CVoxel::Visibility operator~(CVoxel::Visibility lhs)
    {
        return static_cast<CVoxel::Visibility>(~static_cast<uint8_t>(lhs));
    }

    using Voxel = CVoxel*; //std::shared_ptr<CVoxel>;
    
    struct SChunk
    {
        size_t UniqueId;    //!< Unique identifier of the chunks. Only changes, if the voxel mesh is resized.
        CBBox TotalBBox;    //!< The total bounding box of the chunk.
        CBBox InnerBBox;    //!< The bounding box of the model inside the chunk.
    };
    
    class CVoxelOctree : public COctree<Voxel>
    {
        public:
            CVoxelOctree();
            CVoxelOctree(const CVoxelOctree &_tree);
            CVoxelOctree(const CVectori &_size, int _depth = 10);

            std::map<CVectori, Voxel> queryVisible(bool opaque);
            std::list<SChunk> queryDirtyChunks();

            std::list<SChunk> queryBBoxes();
            void generateVisibilityMask();

            void updateVisibility(const CVectori &_Position);

            ~CVoxelOctree() = default;

        private:
            void CheckVisibility(Voxel _v, const CVectori &_dir, CVoxel::Visibility lhs, CVoxel::Visibility rhs);

            void CheckVisibility(const Voxel &_v, const Voxel &_v2, char _axis);
    };

    enum class VoxelMode : uint8_t
    {
        KEEP_ALL,
        KEEP_ONLY_VISIBLE = 1
    };

    class CVoxelMesh
    {
        using VoxelData = CVoxelOctree;//COctree<Voxel>;

        public:
            // For the gui
            std::string Name;
            Texture Thumbnail;

            CVector Pivot;
            CBBox BBox;

            std::vector<Material> Materials;                //!< Used materials
            std::map<TextureType, Texture> Colorpalettes;   //!< Used colors / textures

            CVoxelMesh(VoxelMode mode = VoxelMode::KEEP_ALL) : m_Mode(mode) 
            { }

            /**
             * @brief Sets the size of the voxel space.
             */
            inline void SetSize(const CVectori &Size)
            {
                m_Size = Size;
                m_Voxels = VoxelData(m_Size, 10);
                m_Pool.clear();
            }

            /**
             * The voxel space begins always at (0, 0, 0) which is the bottom, front left corner.
             * The z axis is the Up axis and y the forward axis.
             * 
             * @return Returns the voxel space size of a voxel mesh.
             */
            inline CVectori GetSize() const
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
                std::lock_guard<std::recursive_mutex> lock(m_Lock);
                BBox = CBBox(CVectori(INT32_MAX, INT32_MAX, INT32_MAX), CVectori(0, 0, 0));

                for (auto &&v : m_Voxels)
                {
                    BBox.Beg = BBox.Beg.Min(v.first);
                    BBox.End = BBox.End.Max(v.first + CVector(1, 1, 1));
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
                std::lock_guard<std::recursive_mutex> lock(m_Lock);
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
            void SetVoxel(const CVectori &Pos, int Material, int Color, bool Transparent, CVoxel::Visibility mask = CVoxel::Visibility::VISIBLE);

            /**
             * @brief Removes a voxel on a given position
             * 
             * @param Pos: Position of the voxel to remove.
             */
            void RemoveVoxel(const CVectori &Pos);

            /**
             * @brief Clears the mesh
             */
            void Clear();

            void ReserveVoxels(size_t _reserve)
            {
                m_Pool.reserve(_reserve);
            }

            /**
             * @return Gets a voxel on a given position.
             */
            Voxel GetVoxel(const CVectori &Pos);

            /**
             * @brief Gets a voxel on a given position.
             * 
             * @param pos: Position of the voxel
             * @param OpaqueOnly: If true than only opaque voxels are returned, otherwise all transparent one.
             */
            Voxel GetVoxel(const CVectori &Pos, bool OpaqueOnly);

            /**
             * @brief Gets the count of all setted blocks.
             */
            inline size_t GetBlockCount() const
            {
                return m_Voxels.size();
            }
            
            ~CVoxelMesh() = default;
        private:   
            const static CVector CHUNK_SIZE;

            void SetNormal(const CVector &Pos, const CVector &Neighbor, bool IsInvisible = true);

            CVectori m_Size;
            CObjectPool<CVoxel> m_Pool;
            
            VoxelData m_Voxels;

            VoxelMode m_Mode;
            mutable std::recursive_mutex m_Lock;
    };

    using VoxelMesh = std::shared_ptr<CVoxelMesh>;

    //////////////////////////////////////////////////
    // CVoxelOctree functions
    //////////////////////////////////////////////////

    inline CVoxelOctree::CVoxelOctree() : COctree<Voxel>() { }
    inline CVoxelOctree::CVoxelOctree(const CVoxelOctree &_tree) : COctree<Voxel>(_tree) { }
    inline CVoxelOctree::CVoxelOctree(const CVectori &_size, int _depth) : COctree<Voxel>(_size, _depth) { }

    inline std::map<CVectori, Voxel> CVoxelOctree::queryVisible(bool opaque)
    {
        std::map<CVectori, Voxel> ret;

        if(this->m_Nodes)
        {
            // Goes the Octree down to the leftmost leave
            internal::COctreeNode<Voxel> *node = this->m_Nodes[0];
            while(node->CanSubdivide() && node->m_Nodes)
                node = node->m_Nodes[0];

            while (node != this)
            {
                // Searches the next cube with content.
                while(node->m_Content.empty())
                {
                    internal::COctreeNode<Voxel> *parent = node->m_Parent;
                    if(!parent)
                        break;

                    // Calculates the current index inside the parent cube.
                    size_t idx = parent->CalcIndex(node->m_BBox.Beg);
                    if(idx < (NODES_COUNT - 1))
                    {
                        idx++;
                        node = parent->m_Nodes[idx];   

                        // Searches for the next leave.
                        while(node->CanSubdivide() && node->m_Nodes)
                            node = node->m_Nodes[0];                
                    }
                    else
                        node = parent;
                }

                if(node != this)
                {
                    for (auto &&v : node->m_Content)
                    {
                        if(v.second->IsVisible() && v.second->Transparent == !opaque)
                            ret.insert(v);
                    }
                
                    // Next cube.
                    size_t idx = node->m_Parent->CalcIndex(node->m_BBox.Beg);
                    if(idx < (NODES_COUNT - 1))
                    {
                        idx++;
                        node = node->m_Parent->m_Nodes[idx];   
                    }
                    else
                        node = node->m_Parent;
                }
            }
        }

        return ret;
    }

    inline std::list<SChunk> CVoxelOctree::queryDirtyChunks()
    {
        std::list<SChunk> ret;
        for (auto &&node : m_ContentNodes)
        {
            if(node->m_IsDirty)
            {
                node->m_IsDirty = false;
                ret.push_back({(size_t)node, node->m_BBox, node->m_InnerBBox});
            }
        }
        

        // if(this->m_Nodes)
        // {
        //     std::list<internal::COctreeNode<Voxel>*> dirtyNodes;

        //     for (size_t i = 0; i < NODES_COUNT; i++)
        //     {
        //         internal::COctreeNode<Voxel> *node = this->m_Nodes[i];
        //         while (node != this)
        //         {
        //             if(!node->m_IsDirty)
        //                 break;

        //             // Searches the next subdirty chunk.
        //             // Goes the current path down to the bottom.
        //             size_t dirtyIdx = 0;
        //             while(node->CanSubdivide() && node->m_Nodes)
        //             {
        //                 auto tmpNode = node->m_Nodes[dirtyIdx];
        //                 if(!tmpNode->m_IsDirty)
        //                 {
        //                     dirtyIdx++;
        //                     if(dirtyIdx > (NODES_COUNT - 1))
        //                         break;

        //                     continue;
        //                 }

        //                 node = tmpNode;
        //                 dirtyIdx = 0;
        //             }

        //             // Add all nodes with content to the list.
        //             if(!node->m_Content.empty())
        //                 dirtyNodes.push_back(node);

        //             // Mark the current node as processed.
        //             node->m_IsDirty = false;
        //             node = node->m_Parent;
        //         }
        //     }

        //     // Add the chunk data to the return value.
        //     for (auto &&node : dirtyNodes)
        //         ret.push_back({(size_t)node, node->m_BBox, node->m_InnerBBox});
        // }

        return ret;
    }

    inline std::list<SChunk> CVoxelOctree::queryBBoxes()
    {
        std::list<SChunk> ret;

        if(this->m_Nodes)
        {
            internal::COctreeNode<Voxel> *node = this->m_Nodes[0];
            while(node->CanSubdivide() && node->m_Nodes)
                node = node->m_Nodes[0];

            while (node != this)
            {
                while(node->m_Content.empty())
                {
                    internal::COctreeNode<Voxel> *parent = node->m_Parent;
                    if(!parent)
                        break;

                    size_t idx = parent->CalcIndex(node->m_BBox.Beg);
                    if(idx < (NODES_COUNT - 1))
                    {
                        idx++;
                        node = parent->m_Nodes[idx];   

                        while(node->CanSubdivide() && node->m_Nodes)
                            node = node->m_Nodes[0];                
                    }
                    else
                        node = parent;
                }

                if(node != this)
                {
                    ret.push_back({(size_t)node, node->m_BBox, node->m_InnerBBox});

                    size_t idx = node->m_Parent->CalcIndex(node->m_BBox.Beg);
                    if(idx < (NODES_COUNT - 1))
                    {
                        idx++;
                        node = node->m_Parent->m_Nodes[idx];   
                    }
                    else
                        node = node->m_Parent;
                }
            }
        }

        return ret;
    }

    inline void CVoxelOctree::updateVisibility(const CVectori &_Position)
    {
        auto IT = this->find(_Position);
        if(IT == this->end())
        {
            const static std::vector<std::pair<CVector, CVoxel::Visibility>> DIRECTIONS = {
                {CVector(1, 0, 0), CVoxel::Visibility::LEFT},
                {CVector(-1, 0, 0), CVoxel::Visibility::RIGHT},
                {CVector(0, 1, 0), CVoxel::Visibility::BACKWARD},
                {CVector(0, -1, 0), CVoxel::Visibility::FORWARD},
                {CVector(0, 0, -1), CVoxel::Visibility::UP},
                {CVector(0, 0, 1), CVoxel::Visibility::DOWN}
            };

            for (auto &&dir : DIRECTIONS)
            {
                IT = this->find(_Position + dir.first);
                if(IT != this->end())
                    IT->second->VisibilityMask |= dir.second;
            }
        }
        else
        {
            const static std::vector<std::pair<CVector, int>> AXIS_DIRECTIONS = {
                {CVector(1, 0, 0), 0},
                {CVector(0, 1, 0), 1},
                {CVector(0, 0, 1), 2}
            };

            for (auto &&axis : AXIS_DIRECTIONS)
            {
                CVector start = _Position - axis.first;
                for (char i = 0; i < 2; i++)
                {
                    IT = this->find(start);
                    auto IT2 = this->find(start + axis.first);

                    if(IT != this->end() && IT2 != this->end())
                        CheckVisibility(IT->second, IT2->second, axis.second);

                    start += axis.first;
                }
            }
        }
    }

    inline void CVoxelOctree::generateVisibilityMask()
    {
        if(!this->m_Nodes)
            return;

        internal::COctreeNode<Voxel> *node = this->m_Nodes[0];
        while(node->CanSubdivide() && node->m_Nodes)
            node = node->m_Nodes[0];

        if(node->m_Content.empty())
            node = internal::NextNoneEmptyNode(node);

        while(node != this)
        {
            CVectori beg = node->m_InnerBBox.Beg;
            CVectori end = node->m_InnerBBox.End;

            for (char axis = 0; axis < 3; axis++)
            {
                int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
                int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
                Voxel current;

                for (int x = beg.v[axis]; x < end.v[axis]; x++)
                {
                    for (int y = beg.v[axis1]; y < end.v[axis1]; y++)
                    {
                        // Reset current
                        CVectori pos;
                        pos.v[axis] = x;
                        pos.v[axis1] = y;
                        pos.v[axis2] = beg.v[axis2];

                        auto it = node->m_Content.find(pos);
                        if(it != node->m_Content.end())
                            current = it->second;
                        else
                            current = nullptr;

                        for (int z = beg.v[axis2] + 1; z < end.v[axis2]; z++)
                        {
                            pos.v[axis] = x;
                            pos.v[axis1] = y;
                            pos.v[axis2] = z;

                            it = node->m_Content.find(pos);
                            if(it != node->m_Content.end())
                            {
                                CheckVisibility(current, it->second, axis2);
                                current = it->second;
                            }
                            else
                                current = nullptr;
                        }

                        if(current && current->Pos.v[axis2] == (node->m_BBox.End.v[axis2] - 1))
                        {
                            pos = current->Pos;
                            pos.v[axis2] += 1;

                            auto ocit = find(pos);
                            if(ocit != this->end())
                                CheckVisibility(current, ocit->second, axis2);
                        }
                    }
                }
            }  

            node = internal::NextNoneEmptyNode(node);          
        }

        // +- 520,789227ms
        // COctree<Voxel>::iterator it = begin();
        // while(it != end())
        // {
        //     CheckVisibility(it->second, CVectori(1, 0, 0), CVoxel::Visibility::RIGHT, CVoxel::Visibility::LEFT);
        //     CheckVisibility(it->second, CVectori(0, 1, 0), CVoxel::Visibility::FORWARD, CVoxel::Visibility::BACKWARD);
        //     CheckVisibility(it->second, CVectori(0, 0, 1), CVoxel::Visibility::UP, CVoxel::Visibility::DOWN);

        //     it++;
        // }
    }

    inline void CVoxelOctree::CheckVisibility(Voxel _v, const CVectori &_dir, CVoxel::Visibility lhs, CVoxel::Visibility rhs)
    {
        auto it = find(CVectori(_v->Pos) + _dir);
        if(it != this->end())
        {
            _v->VisibilityMask &= ~lhs; //CVoxel::Visibility::RIGHT;
            it->second->VisibilityMask &= ~rhs; //CVoxel::Visibility::LEFT;
        }
    }

    inline void CVoxelOctree::CheckVisibility(const Voxel &_v, const Voxel &_v2, char _axis)
    {
        const static std::pair<CVoxel::Visibility, CVoxel::Visibility> ADJACENT_FACES[3] = {
            {~CVoxel::Visibility::RIGHT, ~CVoxel::Visibility::LEFT},
            {~CVoxel::Visibility::FORWARD, ~CVoxel::Visibility::BACKWARD},
            {~CVoxel::Visibility::UP, ~CVoxel::Visibility::DOWN},
        };

        if(_v)
        {
            // 1. Both opaque touching faces invisible
            // 2. One transparent touching faces visible
            // 3. Both transparent different transparency faces visible
            // 4. Both transparent same transparency and color faces invisible
            // 5. Both transparent different transparency and same color faces visible

            bool hideFaces = false;
            if(!_v2->Transparent && !_v->Transparent)
                hideFaces = true;
            else if(_v2->Transparent && !_v->Transparent || !_v2->Transparent && _v->Transparent)
                hideFaces = false;
            else if(_v2->Transparent && _v->Transparent)
            {
                if(_v2->Material != _v->Material)
                    hideFaces = false;
                else if(_v2->Color != _v->Color)
                    hideFaces = false;
                else
                    hideFaces = true;
            }

            if(hideFaces)
            {
                const std::pair<CVoxel::Visibility, CVoxel::Visibility> &adjacent_faces = ADJACENT_FACES[_axis];

                _v->VisibilityMask &= adjacent_faces.first;
                _v2->VisibilityMask &= adjacent_faces.second;
            }
        }
    }
} // namespace VoxelOptimizer


#endif //VOXELMESH_HPP