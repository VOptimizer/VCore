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
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/Misc/FileStream.hpp>

#include <VCore/Meshing/Material.hpp>

namespace VCore
{
    enum class VoxelFormatType
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
            VoxelSceneTree SceneTree; //!< Scene tree of this voxel file.

            IVoxelFormat() : m_Mode(FileMode::CLOSED), m_IOHandler(nullptr), m_DataStream(nullptr) {}

            /**
             * @brief Creates an instance for a voxel format.
             * The underlying loader is determined by the given file.
             * 
             * @tparam IOHandler: Type of an io loader which extends ::IIOHandler
             * @param p_Filename: Path to a voxel file
             * @param p_Mode: Mode to open the given file. See ::FileMode for more informations
             * @throw CVoxelFormatException: If no loader is found
             */
            template<class IOHandler = CDefaultIOHandler>
            static VoxelFormat CreateAndOpen(const std::string &p_Filename, FileMode p_Mode)
            {
                auto loader = Create(GetType(p_Filename));
                loader->Open<IOHandler>(p_Filename, p_Mode);

                return loader;
            }
      
            /**
             * @brief Opens a voxel file
             * 
             * @tparam IOHandler: Type of an io loader which extends ::IIOHandler
             * @param p_Filename: Path to a voxel file
             * @param p_Mode: Mode to open the given file. See ::FileMode for more informations
             */
            template<class IOHandler = CDefaultIOHandler>
            void Open(const std::string &p_File, FileMode p_Mode)
            {
                Open(new IOHandler(), p_File, p_Mode);
            }

            /**
             * @brief Opens a voxel file
             * 
             * @param p_IOHandler: IOHandler instance, which is used to create file streams.
             * @param p_Filename: Path to a voxel file
             * @param p_Mode: Mode to open the given file. See ::FileMode for more informations
             */
            virtual void Open(IIOHandler *p_IOHandler, const std::string p_File, FileMode p_Mode);

            /** Loads the voxel file */
            virtual void Load();

            /** Write the SceneTree to file. */
            virtual void Save();

            /** Closes the file */
            virtual void Close();

            /**
             * @return Returns the loader type of a given file.
             */
            static VoxelFormatType GetType(const std::string &p_Filename);

            /**
             * @brief Creates an instance of a the given loader;
             *
             * @throw CVoxelFormatException: If no loader is found
             */
            static VoxelFormat Create(VoxelFormatType p_Type);

            virtual ~IVoxelFormat() { ClearCache(); DeleteFileStream(); }
        protected:
            virtual void ClearCache();
            void DeleteFileStream();

            FileMode m_Mode;

            std::shared_ptr<IIOHandler> m_IOHandler;
            IFileStream *m_DataStream;

            virtual void ParseFormat() = 0;
            virtual void WriteFormat() {}
    };
}


#endif //IVOXELFORMAT_HPP