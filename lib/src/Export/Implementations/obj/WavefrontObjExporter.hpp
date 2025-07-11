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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WAVEFRONTOBJEXPORTER_HPP
#define WAVEFRONTOBJEXPORTER_HPP

#include "VCore/Misc/unordered_dense.h"
#include <VCore/Export/IExporter.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <cstdint>

namespace VCore 
{
  class CWavefrontObjExporter : public IExporter 
  {
    public:
      CWavefrontObjExporter() = default;
      ~CWavefrontObjExporter() = default;
    private:
      IFileStream *m_ObjFile{};
      IFileStream *m_MtlFile{};
      ankerl::unordered_dense::set<uint32_t> m_Colors;
      ankerl::unordered_dense::map<uint8_t, uint8_t> m_Materials;
      std::string m_FilenameWithoutExt;
      uint64_t m_IndexOffset{};

      void WriteHeaderData(const fast_vector<Mesh> &) override;
      bool SupportsSceneTree() override { return false; }
      void WriteMeshData(const Mesh &p_Mesh) override;
      void WriteFooterData() override;

      void EnterSceneNode(const CSceneNodeBase *) override {}
      void LeaveSceneNode(const CSceneNodeBase *) override {}

      std::string GetObjMaterial(const uint8_t p_MaterialHandle);

      void GenerateTextureAndPatchUV();
      uint64_t GetTextureSizeP2();
  };
}

#endif // WAVEFRONTOBJEXPORTER_HPP