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

#include <string>
#include <VCore/Meshing/Color.hpp>

namespace VCore
{
    class CMaterial
    {
        public:
            CMaterial() : Name(), Metallic(0), Specular(0), Roughness(1), IOR(0), Emission(0), Power(0), Transparency(0) {}
            CMaterial(const CMaterial &p_Material) { *this = p_Material; }

            std::string Name;
            float Metallic;
            float Specular;
            float Roughness;
            float IOR;
            float Emission;
            float Power;    //!< For emissive.
            float Transparency;

            inline CMaterial& operator=(const CMaterial &p_Material)
            {
                Name = p_Material.Name;
                Metallic = p_Material.Metallic;
                Specular = p_Material.Specular;
                Roughness = p_Material.Roughness;
                IOR = p_Material.IOR;
                Emission = p_Material.Emission;
                Power = p_Material.Power;
                Transparency = p_Material.Transparency;

                return *this;
            }

            inline bool operator==(const CMaterial &p_Other) const
            {
                bool equal = false;
                equal = Name == p_Other.Name;
                equal = Metallic == p_Other.Metallic && equal;
                equal = Specular == p_Other.Specular && equal;
                equal = Roughness == p_Other.Roughness && equal;
                equal = IOR == p_Other.IOR && equal;
                equal = Emission == p_Other.Emission && equal;
                equal = Power == p_Other.Power && equal;
                equal = Transparency == p_Other.Transparency && equal;

                return equal;
            }

            inline bool operator!=(const CMaterial &p_Other) const { return !operator==(p_Other); }

            ~CMaterial() = default;
    };

    using Material = CMaterial*;
}

#endif //MATERIAL_HPP