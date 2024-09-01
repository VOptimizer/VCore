/*
 * MIT License
 *
 * Copyright (c) 2024 Christian Tost
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

#ifndef SLICES_HPP
#define SLICES_HPP

#include <map>
#include <VCore/Misc/unordered_dense.h>
#include <vector>
#include <VCore/Math/Vector.hpp>
#include <VCore/Meshing/Texture.hpp>

namespace VCore
{
    class CQuadInfo;

    using Quad = std::pair<Math::Vec3i, Math::Vec3i>;
    using Quads = std::vector<CQuadInfo>;
    using Slice = std::map<int, Quads>;
    using Slices = ankerl::unordered_dense::map<int, Slice>;

    class CQuadInfo
    {
        public:
            CQuadInfo() = default;
            CQuadInfo(const Quad &_Quad, const Math::Vec3i &_Normal, int _Material, int _Color) : mQuad(_Quad), Normal(_Normal), Material(_Material), Color(_Color) {}
            CQuadInfo(const Quad &_Quad, const Math::Vec3i &_Normal, int _Material, const std::map<TextureType, std::vector<CColor>> &_RawTextures) : CQuadInfo(_Quad, _Normal, _Material, 0) { RawTextures = _RawTextures; }
            CQuadInfo(CQuadInfo &&_Other) { *this = std::move(_Other); }
            CQuadInfo(const CQuadInfo &_Other) { *this = _Other; }

            CQuadInfo &operator=(CQuadInfo &&_Other)
            {
                mQuad = _Other.mQuad;
                Normal = std::move(_Other.Normal);
                Material = std::move(_Other.Material);
                Color = std::move(_Other.Color);
                RawTextures = std::move(_Other.RawTextures);
                return *this;
            }

            CQuadInfo &operator=(const CQuadInfo &_Other)
            {
                mQuad = _Other.mQuad;
                Normal = _Other.Normal;
                Material = _Other.Material;
                Color = _Other.Color;
                RawTextures = _Other.RawTextures;
                return *this;
            }

            Quad mQuad;
            Math::Vec3i Normal;
            int Material;
            int Color;
            Math::Vec2ui UvStart;

            std::map<TextureType, std::vector<CColor>> RawTextures;
    };

    class CSliceCollection
    {
        public:
            Slices mSlices[3];
            ankerl::unordered_dense::map<TextureType, Texture> Textures;

            CSliceCollection() = default;
            CSliceCollection(CSliceCollection &&_Other) { *this = std::move(_Other); }

            CSliceCollection &operator=(CSliceCollection &&_Other)
            {
                for (size_t i = 0; i < 3; i++)
                    mSlices[i] = std::move(_Other.mSlices[i]);
                Textures = std::move(_Other.Textures);
                return *this;
            }

            /**
             * @brief Tries to combine as many quads into one as possible.
             */
            void Optimize(bool _GenerateTexture);

            /**
             * @brief Merges an another collection into this one.
             */
            void Merge(const CSliceCollection &_Other);

            /**
             * @brief Adds a new slice to the given axis.
             */
            void AddSlice(int _Axis, int _Depth);

            /**
             * @brief Adds a new quad info to a given axis.
             */
            void AddQuadInfo(int _Axis, int _Depth, int _Height, const CQuadInfo &_Info);

        private:
            /**
             * @brief Finds the best spot to insert other quads later.
             * Uses a binary search operation to quickly find the position.
            */
            Quads::const_iterator FindInsertionPoint(const Quads &_Haystack, const Math::Vec3i &_Pos);

            Quads::const_iterator FindQuad(const Quads &_Haystack, const Math::Vec3i &_Pos);
    };
} // namespace VCore


#endif