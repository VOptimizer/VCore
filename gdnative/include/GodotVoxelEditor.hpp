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

#ifndef GODOTVOXELEDITOR_HPP
#define GODOTVOXELEDITOR_HPP

#include <ArrayMesh.hpp>
#include <Godot.hpp>
#include <Intersection.hpp>
#include <MeshCache.hpp>
#include <Reference.hpp>
#include <VoxelOptimizer/VoxelOptimizer.hpp>
#include <vector>

using namespace godot;

class CGodotVoxelEditor : public Reference
{
    GODOT_CLASS(CGodotVoxelEditor, Reference);
    public:
        CGodotVoxelEditor() = default;

        void _init() { }
        static void _register_methods();

        /**
         * @return Returns an intersection object.
         */
        Ref<CIntersection> Intersects(Vector3 _Pos, Vector3 _Dir) const;

        /// @brief Adds a voxel to the mesh, or changes the color of a already existing one.
        /// @param _Pos Position in the voxel space
        /// @param _Color Color of the voxel
        void AddVoxel(Vector3 _Pos, Color _Color);

        /// @brief Removes a voxel
        /// @param _Pos Position of the voxel to remove
        void RemoveVoxel(Vector3 _Pos);

        // Godot properties wrappers.

        inline Ref<CMeshCache> GetMeshCache() const
        {
            return m_MeshCache;
        }

        inline void SetMeshCache(Ref<CMeshCache> _MeshCache)
        {
            m_MeshCache = _MeshCache;
        }

        ~CGodotVoxelEditor() = default;

    private:
        void RegenerateMesh();

        Ref<CMeshCache> m_MeshCache;
};

#endif