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

#include <VCore/Meshing/Material.hpp>

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

    enum class FileMode
    {
        CLOSED = 0,
        READ = 1,
        WRITE = 2,
        STREAMED = 4
    };

    class IVoxelFormat;
    using VoxelFormat = std::shared_ptr<IVoxelFormat>;

    class IVoxelFormat
    {
        public:
            IVoxelFormat() : m_Mode(FileMode::CLOSED), m_IOHandler(nullptr), m_DataStream(nullptr) {}

            template<class IOHandler = CDefaultIOHandler>
            static VoxelFormat CreateAndOpen(const std::string &_Filename, FileMode _Mode)
            {
                auto loader = Create(GetType(_Filename));
                loader->Open<IOHandler>(_Filename, _Mode);

                return loader;
            }
      
            template<class IOHandler = CDefaultIOHandler>
            void Open(const std::string &_File, FileMode _Mode)
            {
                Open(new IOHandler(), _File, _Mode);
            }

            virtual void Open(IIOHandler *_IOHandler, const std::string _File, FileMode _Mode);

            virtual void Load();
            virtual void Save();

            virtual void Close();

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

            std::vector<VoxelModel> m_Models;

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

            virtual ~IVoxelFormat() { ClearCache(); DeleteFileStream(); }
        protected:
            virtual void ClearCache();
            void DeleteFileStream();

            SceneNode m_SceneTree;
            FileMode m_Mode;

            std::shared_ptr<IIOHandler> m_IOHandler;
            IFileStream *m_DataStream;

            
            std::vector<VoxelAnimation> m_Animations;
            std::vector<Material> m_Materials;
            ankerl::unordered_dense::map<TextureType, Texture> m_Textures;

            virtual void ParseFormat() = 0;
            virtual void WriteFormat() {}
    };
}


#endif //IVOXELFORMAT_HPP