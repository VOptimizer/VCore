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
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/Voxel/VoxelAnimation.hpp>
#include <VCore/Misc/FileStream.hpp>

namespace VCore
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
            template<class IOHandler = CDefaultIOHandler>
            static VoxelFormat CreateAndLoad(const std::string &_Filename)
            {
                auto loader = Create(GetType(_Filename));
                loader->Load<IOHandler>(_Filename);

                return loader;
            }

            /**
             * @return Returns the loader type of a given file.
             */
            static LoaderType GetType(const std::string &_Filename);

            /**
             * @brief Creates an instance of a the given loader;
             */
            static VoxelFormat Create(LoaderType _Type);

            /**
             * @brief Loads a voxel file from disk.
             * 
             * @param _File: Path to the voxel file.
             * @throws CVoxelLoaderException If the file couldn't be load.
             */
            template<class IOHandler = CDefaultIOHandler>
            void Load(const std::string &_File)
            {
                Load(new IOHandler(), _File);
            }

            /**
             * @brief Loads a voxel file using a given io handler.
             * The loader takes the ownership of the _Strm instance, and will free it properly.
             * 
             * @param _IOHandler: IOHandler to use.
             * @param _File: File to load.
             * @throws CVoxelLoaderException If the file couldn't be load.
             */
            virtual void Load(IIOHandler *_IOHandler, const std::string _File);

            /**
             * @return Gets a list with all models inside the voxel file.
             */
            inline std::vector<VoxelModel> GetModels() const
            {
                return m_Models;
            }

            /**
             * @return Gets a list with all animations of the voxel file.
             */
            inline std::vector<VoxelAnimation> GetAnimations() const
            {
                return m_Animations;
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
             * @return Gets the scene tree of this file.
             */
            inline SceneNode GetSceneTree() const
            {
                return m_SceneTree;
            }

            /**
             * @brief Sets the scene tree.
             */
            inline void SetSceneTree(SceneNode _Tree)
            {
                m_SceneTree = _Tree;
            }

            virtual ~IVoxelFormat() { DeleteFileStream(); }

            /**
             * @brief Merges all colors and materials into one list. With this helper method it is possible to merge multiple voxel files together.
             * 
             * @param textures: Map in which the colors are merged into
             * @param materials: vector in which the materials are merged into
             * @param meshes: Voxel meshes from another file
             */
            // static void Combine(std::map<TextureType, Texture> &textures, std::vector<Material> &materials, const std::vector<VoxelMesh> &meshes);
        protected:
            virtual void ClearCache();
            void DeleteFileStream();

            SceneNode m_SceneTree;

            IIOHandler *m_IOHandler;
            IFileStream *m_DataStream;

            std::vector<VoxelModel> m_Models;
            std::vector<VoxelAnimation> m_Animations;
            std::vector<Material> m_Materials;
            std::map<TextureType, Texture> m_Textures;

            virtual void ParseFormat() = 0;

            // template<class T>
            // T ReadData()
            // {
            //     T Ret;
            //     ReadData((char*)&Ret, sizeof(Ret));
            //     return Ret;
            // }

            // void ReadData(char *Buf, size_t Size);
            // bool IsEof();
            // size_t Tellg();
            // void Skip(size_t Bytes);
            // void Reset();
            // std::vector<char> ReadDataChunk(size_t size);
    };
}


#endif //IVOXELFORMAT_HPP