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

#ifndef MATERIALMANAGER_HPP
#define MATERIALMANAGER_HPP

#include <Godot.hpp>
#include <SpatialMaterial.hpp>
#include <memory>
#include <map>
#include <VoxelOptimizer/VoxelOptimizer.hpp>

using namespace godot;

class CMaterialManager
{
    public:
        CMaterialManager() = default;

        static std::shared_ptr<CMaterialManager> GetInstance();

        /**
         * @brief Creates unknown materials and returns a list of godot materials.
         */
        const std::map<int, Ref<SpatialMaterial>> &CreateAndGetMaterials(const VoxelOptimizer::Mesh &_Mesh);

        /**
         * @brief Clears all materials.
         */
        void Clear();

        ~CMaterialManager() = default;
    private:
        static std::shared_ptr<CMaterialManager> m_Instance;

        std::map<int, Ref<SpatialMaterial>> m_Materials;
};

#endif