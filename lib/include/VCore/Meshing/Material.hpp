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

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <memory>
#include <string>
#include <VCore/Meshing/Color.hpp>

namespace VCore
{
    class CMaterial
    {
        public:
            CMaterial() : Name(), Metallic(0), Specular(0), Roughness(1), IOR(0), Power(0), Transparency(0) {}
            CMaterial(const CMaterial &_Material) { *this = _Material; }

            std::string Name;
            float Metallic;
            float Specular;
            float Roughness;
            float IOR;
            float Power;    //!< For emissive.
            float Transparency;

            inline CMaterial& operator=(const CMaterial &_Material)
            {
                Name = _Material.Name;
                Metallic = _Material.Metallic;
                Specular = _Material.Specular;
                Roughness = _Material.Roughness;
                IOR = _Material.IOR;
                Power = _Material.Power;
                Transparency = _Material.Transparency;

                return *this;
            }

            inline bool operator==(const CMaterial &other)
            {
                bool equal = false;
                equal = Name == other.Name;
                equal = Metallic == other.Metallic && equal;
                equal = Specular == other.Specular && equal;
                equal = Roughness == other.Roughness && equal;
                equal = IOR == other.IOR && equal;
                equal = Power == other.Power && equal;
                equal = Transparency == other.Transparency && equal;

                return equal;
            }

            inline bool operator!=(const CMaterial &other)
            {
                return !operator==(other);
            }

            ~CMaterial() = default;
    };

    using Material = std::shared_ptr<CMaterial>;
}

#endif //MATERIAL_HPP