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

#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP

#include <ArrayMesh.hpp>
#include <Godot.hpp>
#include <Reference.hpp>
#include <vector>

using namespace godot;

/** Contains the position and the normal of a voxel mouse intersection */
class CIntersection : public Reference
{
    GODOT_CLASS(CIntersection, Reference);
    public:
        CIntersection() = default;

        void _init() { }

        static void _register_methods()
        {
            register_property<CIntersection, Vector3>("position", nullptr, &CIntersection::GetPosition, Variant());
            register_property<CIntersection, Vector3>("normal", nullptr, &CIntersection::GetNormal, Variant());
        }

        inline Vector3 GetPosition() const
        {
            return m_Position;
        }

        inline Vector3 GetNormal() const
        {
            return m_Normal;
        }

        inline void SetPosition(const Vector3 &_Position)
        {
            m_Position = _Position;
        }

        inline void SetNormal(const Vector3 &_Normal)
        {
            m_Normal = _Normal;
        }

        ~CIntersection() = default;
    private:
        Vector3 m_Position;
        Vector3 m_Normal;
};

#endif