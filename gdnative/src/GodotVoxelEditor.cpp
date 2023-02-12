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

#include <GodotVoxelEditor.hpp>
#include <Helpers/VectorUtils.hpp>

const static std::vector<std::pair<VoxelOptimizer::CVoxel::Visibility, VoxelOptimizer::CVector>> NORMALS = {
    {VoxelOptimizer::CVoxel::Visibility::UP, VoxelOptimizer::CVoxel::FACE_UP},
    {VoxelOptimizer::CVoxel::Visibility::DOWN, VoxelOptimizer::CVoxel::FACE_DOWN},
    {VoxelOptimizer::CVoxel::Visibility::LEFT, VoxelOptimizer::CVoxel::FACE_LEFT},
    {VoxelOptimizer::CVoxel::Visibility::RIGHT, VoxelOptimizer::CVoxel::FACE_RIGHT},
    {VoxelOptimizer::CVoxel::Visibility::FORWARD, VoxelOptimizer::CVoxel::FACE_FORWARD},
    {VoxelOptimizer::CVoxel::Visibility::BACKWARD, VoxelOptimizer::CVoxel::FACE_BACKWARD}
};

void CGodotVoxelEditor::_register_methods()
{
    register_method("intersects", &CGodotVoxelEditor::Intersects);
    register_method("add_voxel", &CGodotVoxelEditor::AddVoxel);
    register_method("remove_voxel", &CGodotVoxelEditor::RemoveVoxel);

    register_property<CGodotVoxelEditor, Ref<CMeshCache>>("mesh", &CGodotVoxelEditor::SetMeshCache, &CGodotVoxelEditor::GetMeshCache, Variant());
}

void CGodotVoxelEditor::AddVoxel(Vector3 _Pos, Color _Color)
{
    auto vvec = GVector3ToVVector(_Pos);
    auto mesh = m_MeshCache->GetVoxelMesh();
    auto colorpalette = mesh->Colorpalettes()[VoxelOptimizer::TextureType::DIFFIUSE];
    auto &pixels = colorpalette->Pixels();

    if(vvec < mesh->GetSize())
    {
        int colorIdx = 0;
        auto color = VoxelOptimizer::CColor(_Color.get_r8(), _Color.get_g8(), _Color.get_b8(), _Color.get_a8());
        auto it = std::find(pixels.begin(), pixels.end(), color.AsRGBA());
        if(it == pixels.end())
        {
            colorpalette->AddPixel(color);
            colorIdx = pixels.size() - 1;
        }
        else
            colorIdx = it - pixels.begin();

        mesh->SetVoxel(vvec, 0, colorIdx, _Color.get_a8() != 255);
        mesh->GetVoxels().updateVisibility(vvec);
        RegenerateMesh();
    }
}

void CGodotVoxelEditor::RemoveVoxel(Vector3 _Pos)
{
    auto vvec = GVector3ToVVector(_Pos);
    auto mesh = m_MeshCache->GetVoxelMesh();

    if(vvec < mesh->GetSize())
    {
        mesh->RemoveVoxel(vvec);
        mesh->GetVoxels().updateVisibility(vvec);
        RegenerateMesh();
    }
}

void CGodotVoxelEditor::RegenerateMesh()
{
    VoxelOptimizer::Mesher mesher = VoxelOptimizer::IMesher::Create(VoxelOptimizer::MesherTypes::GREEDY);
    auto chunks = mesher->GenerateMeshes(m_MeshCache->GetVoxelMesh(), true);
    
}

Ref<CIntersection> CGodotVoxelEditor::Intersects(Vector3 _Pos, Vector3 _Dir) const
{
    Ref<CIntersection> intersection = CIntersection::_new();

    auto mesh = m_MeshCache->GetVoxelMesh();
    if(!mesh)
        return intersection;

    auto voxels = mesh->GetVoxels();
    auto pr = GVector3ToVVector(_Pos);
    auto ur = GVector3ToVVector(_Dir.normalized());
    auto w = pr + ur;

    float dist = 1000000;

    for (auto &&v : voxels)
    {
        if(/*v.first < w ||*/ !v.second->IsVisible())
            continue;

        auto Voxel = v.second;

        for (auto &&face : NORMALS)        
        {
            // Checks if the face is invisible or if the camera faces in the same direction.
            if(((Voxel->VisibilityMask & face.first) != face.first) || (ur.Dot(face.second) > 0))
                continue;

            // Helper to remove the active axis.
            auto rn = VoxelOptimizer::CVector(1, 1, 1) - face.second.Abs();

            // Calculates the vertex on the plane.
            auto p = Voxel->Pos;
            if(face.second > VoxelOptimizer::CVoxel::FACE_ZERO)
                p += face.second;

            auto thit = (p - pr).Dot(face.second) / ur.Dot(face.second);
            if(0 < thit && thit < INFINITY)
            {
                auto Xhit = pr + ur * thit;

                // Creates a 2D AABB to check for interceptions.
                auto beg = p;
                auto end = beg + VoxelOptimizer::CVector(1, 1, 1) * rn;

                // Checks for interceptions.
                bool begComp = true, endComp = true;
                for(char i = 0; i < 3; i++)
                {
                    if(rn.v[i] != 0)
                    {
                        begComp = begComp && Xhit.v[i] > beg.v[i];
                        endComp = endComp && Xhit.v[i] <= end.v[i];
                    }
                }

                // Returns the interception.
                if(begComp && endComp)
                {
                    float newDist = std::min(dist, thit);

                    if(newDist != dist)
                    {
                        dist = newDist;

                        intersection->SetPosition(VVectorToGVector3(Voxel->Pos));
                        intersection->SetNormal(VVectorToGVector3(face.second));
                    }
                }             
            }
        }
    }

    return intersection;
}