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

#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Export/ExportSettings.hpp>
#include <cstdint>
#include <memory>
#include <VCore/Meshing/Mesh/Mesh.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <string>

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
        FBX,
        USDC,
        USDA,
        USDZ
    };

    class IExporter : public ISceneTreeVisitor<Mesh>
    {
        public:
            ExportSettings Settings;

            IExporter();

            /**
             * @brief Creates a new exporter instance.
             */
            static Exporter Create(ExporterType p_Type);

            /**
             * @return Returns the exporter type of a given file.
             */
            static ExporterType GetType(const std::string &p_Filename);

            /**
             * @brief Saves given meshdata to a file.
             * 
             * @param p_Path: Path where to save the data.
             * @param p_MeshData: Mesh data to save.
             */
            template<class IOHandler = CDefaultIOHandler, class T>
            void Save(const std::string &p_Path, const T &p_MeshData)
            {
                Save(new IOHandler(), p_Path, p_MeshData);
            }

            /**
             * @brief Generates and saves a mesh.
             * 
             * @param p_Handler: IOHandler instance.
             * @param p_Path: Path of the file.
             * @param p_Mesh: Mesh to save.
             */
            virtual void Save(IIOHandler *p_Handler, const std::string &p_Path, Mesh p_Mesh);

            /**
             * @brief Generates and saves a list of meshes.
             * 
             * @param p_Handler: IOHandler instance.
             * @param p_Path: Path of the file.
             * @param p_Meshes: Meshes to save.
             */
            virtual void Save(IIOHandler *p_Handler, const std::string &p_Path, const fast_vector<Mesh> &p_Meshes);

            /**
             * @brief Generates and saves a list of meshes.
             * 
             * @param p_Handler: IOHandler instance.
             * @param p_Path: Path of the file.
             * @param p_RenderTree: Meshes to save.
             */
            virtual void Save(IIOHandler *p_Handler, const std::string &p_Path, const RenderSceneTree &p_RenderTree);

            virtual ~IExporter() { DeleteFileStream(); }
        
        protected:
            void TraverseTree() override;
            void TraverseNode(const CSceneNodeBase *p_Node) override;

            void WriteSceneNode(const CSceneNodeBase *p_Node, const fast_vector<Mesh> &p_Meshes);
            void WriteMeshes(const fast_vector<Mesh> &p_Meshes);

            /**
             * @brief Called everytime, before any model or node should be written.
             * @param p_Path: Path to write the file to.
             */
            virtual void WriteHeaderData() = 0;

            /** @return Returns true, if the format supports a scene tree */
            virtual bool SupportsSceneTree() = 0;

            // /** @brief Called everytime a child node is entered. Here you can write the node to file and can setup the child parent hierarchy. */
            // virtual void EnterChildNode(const CSceneNodeBase *p_Node) = 0;

            // /** @brief Called everytime a child node is leaved. Here you can setup the child parent hierarchy. */
            // virtual void LeaveChildNode(const CSceneNodeBase *p_Node) = 0;

            /** Writes a mesh to file */
            virtual void WriteMeshData(const Mesh &p_Mesh) = 0;

            /** Writes the footer data of the file. */
            virtual void WriteFooterData() {}

            void CalculateModelDec(uint32_t p_ModelId);



            std::string GetMeshName(Mesh p_Mesh, const std::string &p_Default = "VoxelModel");
            void SaveTexture(const Texture &p_Texture, const std::string &p_Path, const std::string &p_Suffix);
            void DeleteFileStream();

            IIOHandler *m_IOHandler;
            std::string m_Path;
            uint32_t m_ModelIdDec{};
            mutable fast_vector<uint32_t> m_NullModels;
    };
}


#endif //IEXPORTER_HPP