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

#ifndef QUBICLEFORMAT_HPP
#define QUBICLEFORMAT_HPP

#include "VCore/Formats/SceneNode.hpp"
#include "VCore/Math/Vector.hpp"
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Formats/IVoxelFormat.hpp>

namespace VCore
{
    class CQubicleFormat : public IVoxelFormat
    {
        public:
            CQubicleFormat() = default;
            ~CQubicleFormat() = default;
        private:
            void ParseFormat() override;

            void LoadNode(CSceneNodeBase *p_Parent);
            void LoadModel(CSceneNodeBase *p_Parent);
            CSceneModelNode *LoadMatrix(CSceneNodeBase *p_Parent);
            void LoadCompound(CSceneNodeBase *p_Parent);

            template<class T>
            Math::TVector3<T> ReadVector()
            {
                Math::TVector3<T> ret;

                ret.x = m_DataStream->Read<T>();
                ret.y = m_DataStream->Read<T>();
                ret.z = m_DataStream->Read<T>();

                return ret;
            }
    };
}


#endif //QUBICLEFORMAT_HPP