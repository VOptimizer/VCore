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

#ifndef GODOTSCENEEXPORTER_HPP
#define GODOTSCENEEXPORTER_HPP

#include <VCore/Meshing/Color.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Export/IExporter.hpp>
#include <cstdint>
#include <string>

namespace VCore
{
    enum GodotVersion
    {
        GODOT3 = 2,
        GODOT4 = 3
    };

    class CGodotSceneExporter : public IExporter
    {
        public:
            CGodotSceneExporter(GodotVersion p_GodotVersion) : IExporter(), m_GodotVersion(p_GodotVersion) {}
            ~CGodotSceneExporter() = default;

        private:
            IFileStream *m_ESCNFile{};
            uint64_t m_ModelIndex{};
            GodotVersion m_GodotVersion;

            void WriteMaterial(uint8_t p_MaterialHandle);
        protected:
            void WriteBytesAsString(const uint8_t* p_Bytes, uint32_t p_Size);
            void WriteColorAsByteString(const CColor &p_Color);
            void WriteGodot3Surface(const ISurface *p_Surface, uint64_t p_SurfaceIdx);
            void WriteGodot4Surface(const ISurface *p_Surface);

            std::string GetNodeName(const CSceneNodeBase *p_Node);
            std::string GetParentName(const CSceneNodeBase *p_Node);

            std::string FormatResourceId(uint64_t p_Id);

            void WriteHeaderData(const fast_vector<Mesh> &p_Meshes) override;

            bool SupportsSceneTree() override { return true; }

            void EnterSceneNode(const CSceneNodeBase *p_Node) override;
            void LeaveSceneNode(const CSceneNodeBase *) override {}

            void WriteMeshData(const Mesh &p_Mesh) override;

            void WriteFooterData() override;
    };
}


#endif //GODOTSCENEEXPORTER_HPP