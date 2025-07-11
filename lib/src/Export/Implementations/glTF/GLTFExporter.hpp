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

#ifndef GLTFEXPORTER_HPP
#define GLTFEXPORTER_HPP

#include <VCore/Export/IExporter.hpp>
#include <cstdint>
#include "Nodes.hpp"
#include <VCore/Meshing/Mesh/Mesh.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <stack>

namespace VCore
{
    class CGLTFExporter : public IExporter
    {
        public:
            CGLTFExporter() = default;
            ~CGLTFExporter() override = default;
        protected:
            GLTF::GLTFDocument m_Document;
            IFileStream *m_BinaryStream{};
            ankerl::unordered_dense::map<uint8_t, uint64_t> m_MaterialHandleMapper;
            std::stack<uint64_t> m_Nodes;

            void WriteHeaderData(const fast_vector<Mesh> &) override;

            bool SupportsSceneTree() override { return true; }

            void EnterSceneNode(const CSceneNodeBase *p_Node) override;
            void LeaveSceneNode(const CSceneNodeBase *p_Node) override;

            void WriteMeshData(const Mesh &p_Mesh) override;

            void WriteFooterData() override;

            uint64_t GetGLTFMaterialHandle(const uint8_t p_MaterialHandle);

            void CloseStream();
    };
}

#endif //GLTFEXPORTER_HPP