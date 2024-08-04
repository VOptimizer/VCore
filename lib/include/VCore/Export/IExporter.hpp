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

#ifndef IEXPORTER_HPP
#define IEXPORTER_HPP

#include <VCore/Export/ExportSettings.hpp>
#include <map>
#include <memory>
#include <VCore/Meshing/Mesh.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <string>
#include <vector>

namespace VCore
{
    class IExporter;
    using Exporter = std::shared_ptr<IExporter>;

    enum class ExporterType
    {
        UNKNOWN,
        OBJ,
        GLTF,
        GLB,
        ESCN,
        PLY,
        FBX
    };

    class IExporter
    {
        public:
            ExportSettings Settings;

            IExporter();

            /**
             * @brief Creates a new exporter instance.
             */
            static Exporter Create(ExporterType _Type);

            /**
             * @return Returns the exporter type of a given file.
             */
            static ExporterType GetType(const std::string &_Filename);

            /**
             * @brief Saves given meshdata to a file.
             * 
             * @param _Path: Path where to save the data.
             * @param _MeshData: Mesh data to save.
             */
            template<class IOHandler = CDefaultIOHandler, class T>
            void Save(const std::string &_Path, const T &_MeshData)
            {
                Save(new IOHandler(), _Path, _MeshData);
            }

            /**
             * @brief Generates and saves a mesh.
             * 
             * @param _Handler: IOHandler instance.
             * @param _Path: Path of the file.
             * @param _Mesh: Mesh to save.
             */
            virtual void Save(IIOHandler *_Handler, const std::string &_Path, Mesh _Mesh);

            /**
             * @brief Generates and saves a list of meshes.
             * 
             * @param _Handler: IOHandler instance.
             * @param _Path: Path of the file.
             * @param _Meshes: Meshes to save.
             */
            virtual void Save(IIOHandler *_Handler, const std::string &_Path, const std::vector<Mesh> &_Meshes);

            virtual ~IExporter() { DeleteFileStream(); }
        
        protected:
            /**
             * @brief Must be implement by the exporter implementation.
             * @param _Path: Path to write the file to.
             * @param _Meshes: List of meshes to write.
             */
            virtual void WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes) = 0;

            std::string GetMeshName(Mesh _Mesh, const std::string & _Default = "VoxelModel");

            void SaveTexture(const Texture &_Texture, const std::string &_Path, const std::string &_Suffix);

            void DeleteFileStream();

            IIOHandler *m_IOHandler;
    };
}


#endif //IEXPORTER_HPP