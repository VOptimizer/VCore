/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
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

#ifndef VECTORUTILS_HPP
#define VECTORUTILS_HPP

#include <Godot.hpp>
#include <VCore/VCore.hpp>

using namespace godot;

inline VCore::CVector GVector3ToVVector(const Vector3 &_Vec)
{
    return VCore::CVector(_Vec.x, _Vec.z, _Vec.y);
}

inline VCore::CVector GVector2ToVVector(const Vector2 &_Vec)
{
    return VCore::CVector(_Vec.x, _Vec.y, 0);
}

inline Vector3 VVectorToGVector3(const VCore::CVector &_Vec)
{
    return Vector3(_Vec.x, _Vec.z, _Vec.y);
}

inline Vector2 GVector2ToVVector(const VCore::CVector &_Vec)
{
    return Vector2(_Vec.x, _Vec.y);
}

#endif