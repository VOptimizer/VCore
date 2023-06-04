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

#ifndef IVOXELFORMAT_HPP
#define IVOXELFORMAT_HPP

#include <string>
#include <vector>
#include <VoxelOptimizer/Formats/SceneNode.hpp>
#include <VoxelOptimizer/Voxel/VoxelMesh.hpp>
#include <VoxelOptimizer/Misc/BinaryStream.hpp>

namespace VoxelOptimizer
{
    enum class LoaderType
    {
        UNKNOWN = -1,
        MAGICAVOXEL,
        GOXEL,
        KENSHAPE,
        QUBICLE_BIN,
        QUBICLE_BIN_TREE,
        QUBICLE_EXCHANGE,
        QUBICLE
    };

    class IVoxelFormat;
    using VoxelFormat = std::shared_ptr<IVoxelFormat>;

    class IVoxelFormat
    {
        public:
            /**
             * @brief Creates an instance of a loader, which then loads the given file.
             * 
             * @throws CVoxelLoaderException If there is no loader for the given file or the file couldn't be load.
             */
            static VoxelFormat CreateAndLoad(const std::string &filename);

            /**
             * @return Returns the loader type of a given file.
             */
            static LoaderType GetType(const std::string &filename);

            /**
             * @brief Creates an instance of a the given loader;
             */
            static VoxelFormat Create(LoaderType type);

            /**
             * @brief Loads a voxel file from disk.
             * 
             * @param File: Path to the voxel file.
             * @throws CVoxelLoaderException If the file couldn't be load.
             */
            virtual void Load(const std::string &File);

            /**
             * @brief Loads voxel file from memory.
             * 
             * @param Data: Data of the file.
             * @param Lenght: Data size.
             * 
             * @throws CVoxelLoaderException If the file couldn't be load.
             */
            virtual void Load(const char *Data, size_t Length);

            /**
             * @return Gets a list with all models inside the voxel file.
             */
            inline std::vector<VoxelMesh> GetModels() const
            {
                return m_Models;
            }

            inline std::map<TextureType, Texture> GetTextures() const
            {
                return m_Textures;
            }

            inline std::vector<Material> GetMaterials() const
            {
                return m_Materials;
            }

            /**
             * @brief Saves the voxel mesh as a voxel file.
             */         
            void Save(const std::string &path, const std::vector<VoxelMesh> &meshes);

            /**
             * @brief Saves the voxel mesh as a voxel file.
             * 
             * @return Returns the file as memory stream.
             */         
            virtual std::vector<char> Save(const std::vector<VoxelMesh> &meshes);

            /**
             * @return Gets the scene tree of this file.
             */
            inline SceneNode GetSceneTree() const
            {
                return m_SceneTree;
            }

            /**
             * @brief Sets the scene tree.
             */
            inline void SetSceneTree(SceneNode tree)
            {
                m_SceneTree = tree;
            }

            /**
             * @brief Merges all colors and materials into one list. With this helper method it is possible to merge multiple voxel files together.
             * 
             * @param textures: Map in which the colors are merged into
             * @param materials: vector in which the materials are merged into
             * @param meshes: Voxel meshes from another file
             */
            static void Combine(std::map<TextureType, Texture> &textures, std::vector<Material> &materials, const std::vector<VoxelMesh> &meshes);
        protected:
            virtual void ClearCache();

            SceneNode m_SceneTree;
            CBinaryStream m_DataStream;

            std::vector<VoxelMesh> m_Models;
            std::vector<Material> m_Materials;
            std::map<TextureType, Texture> m_Textures;

            virtual void ParseFormat() = 0;

            template<class T>
            T ReadData()
            {
                T Ret;
                ReadData((char*)&Ret, sizeof(Ret));
                return Ret;
            }

            void ReadData(char *Buf, size_t Size);
            bool IsEof();
            size_t Tellg();
            void Skip(size_t Bytes);
            void Reset();
            std::vector<char> ReadDataChunk(size_t size);
    };
}


#endif //IVOXELFORMAT_HPP