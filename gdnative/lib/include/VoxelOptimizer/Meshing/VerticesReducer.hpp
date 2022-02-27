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

#ifndef VERTICESREDUCER_HPP
#define VERTICESREDUCER_HPP

#include <VoxelOptimizer/Meshing/Mesh.hpp>
#include <VoxelOptimizer/Memory/Allocator.hpp>
#include <list>

namespace VoxelOptimizer 
{
    class CVerticesReducer
    {
        public:
            CVerticesReducer(/* args */) {}

            Mesh Reduce(Mesh mesh);

            ~CVerticesReducer() {}

        private:
            class CTriangle
            {
                public:
                    std::vector<CVector> Indices;
                    Material Mat;
            };
            using Triangle = std::shared_ptr<CTriangle>;
            class Point
            {
                public:
                    Point(const CVector& position, const CVector &index) : Position(position), Index(index) {}

                    inline void CalcAngle(const CVector& center, const CVector& normal)
                    {
                        // char idx = 0, switcher = 0;
                        // CVector u, v;

                        // for(char c = 0; c < 3; c++)
                        // {
                        //     if(fabs(normal.v[c]) == 0)
                        //     {
                        //         Position2D.v[idx] = Position.v[c];
                        //         idx++;

                        //         if(switcher == 0)
                        //         {
                        //             u.v[c] = 1;
                        //             switcher++;
                        //         }
                        //         else
                        //             v.v[c] = 1;
                        //     }
                        // }

                        // CVector pc(u.Dot(center), v.Dot(center), 0);
                        // m_Angle = atan2(Position2D.y - pc.y, Position2D.x - pc.x);

                        // CVector forward(0, 0, 1);
                        // CVector right(1, 0, 0);
                        // CVector u;

                        // CVector n = normal.Normalize();

                        // // Projects the 3D point onto a 2D plane
                        // //https://answers.unity.com/questions/1522620/converting-a-3d-polygon-into-a-2d-polygon.html
                        // if(fabs(forward.Dot(n)) < 0.2f)
                        //     u = forward - forward.Dot(n) * n;
                        // else
                        //     u = right - right.Dot(n) * n;
                            
                        // // u = (Position.Normalize() - Position.Normalize().Dot(n) * n);

                        // CVector v = u.Cross(n); //.Normalize();

                        // CVector p(Position.Dot(u), Position.Dot(v), 0);
                        // CVector pc(u.Dot(center), v.Dot(center), 0);

                        // Position2D = p;
                        // m_Angle = atan2(p.y - pc.y, p.x - pc.x);

                        // Position - Position.Dot(n) * n;

                        int idx = 0;

                        convert_xyz_to_cube_uv(normal.x, normal.y, normal.z, Position.x, Position.y, Position.z, &idx, &Position2D.x, &Position2D.y);

                        CVector pc;
                        convert_xyz_to_cube_uv(normal.x, normal.y, normal.z, center.x, center.y, center.z, &idx, &pc.x, &pc.y);


                        m_Angle = atan2(Position2D.y - pc.y, Position2D.x - pc.x);
                    }

                    inline void convert_xyz_to_cube_uv(float nx, float ny, float nz, float x, float y, float z, int *index, float *u, float *v)
                    {
                        float absX = fabs(nx);
                        float absY = fabs(ny);
                        float absZ = fabs(nz);
                        
                        int isXPositive = nx > 0 ? 1 : 0;
                        int isYPositive = ny > 0 ? 1 : 0;
                        int isZPositive = nz > 0 ? 1 : 0;
                        
                        float maxAxis, uc, vc;
                        
                        // POSITIVE X
                        if (isXPositive && absX >= absY && absX >= absZ) {
                            // u (0 to 1) goes from +z to -z
                            // v (0 to 1) goes from -y to +y
                            maxAxis = absX;
                            uc = -z;
                            vc = y;
                            *index = 0;
                        }
                        // NEGATIVE X
                        if (!isXPositive && absX >= absY && absX >= absZ) {
                            // u (0 to 1) goes from -z to +z
                            // v (0 to 1) goes from -y to +y
                            maxAxis = absX;
                            uc = z;
                            vc = y;
                            *index = 1;
                        }
                        // POSITIVE Y
                        if (isYPositive && absY >= absX && absY >= absZ) {
                            // u (0 to 1) goes from -x to +x
                            // v (0 to 1) goes from +z to -z
                            maxAxis = absY;
                            uc = x;
                            vc = -z;
                            *index = 2;
                        }
                        // NEGATIVE Y
                        if (!isYPositive && absY >= absX && absY >= absZ) {
                            // u (0 to 1) goes from -x to +x
                            // v (0 to 1) goes from -z to +z
                            maxAxis = absY;
                            uc = x;
                            vc = z;
                            *index = 3;
                        }
                        // POSITIVE Z
                        if (isZPositive && absZ >= absX && absZ >= absY) {
                            // u (0 to 1) goes from -x to +x
                            // v (0 to 1) goes from -y to +y
                            maxAxis = absZ;
                            uc = x;
                            vc = y;
                            *index = 4;
                        }
                        // NEGATIVE Z
                        if (!isZPositive && absZ >= absX && absZ >= absY) {
                            // u (0 to 1) goes from +x to -x
                            // v (0 to 1) goes from -y to +y
                            maxAxis = absZ;
                            uc = -x;
                            vc = y;
                            *index = 5;
                        }

                        // Convert range from -1 to 1 to 0 to 1
                        // *u = 0.5f * (uc / maxAxis + 1.0f);
                        // *v = 0.5f * (vc / maxAxis + 1.0f);

                        *u = uc;
                        *v = vc;
                    }

                    CVector Position;
                    CVector Position2D;
                    CVector Index;
                    
                    bool operator<(const Point &p) const {
                        return m_Angle < p.m_Angle;
                    }

                private:
                    double m_Angle;
            };

            class CPolygon
            {
                public:
                    // CPolygon(const std::list<Triangle> &_tris, const CVector &_curIdx, const std::vector<CVector> &_verts, const CVector &normal);
                    CPolygon(const std::list<Triangle> &_tris, const std::vector<CVector> &_verts, const CVector &normal);

                    /**
                     * @return Returns true if the polygon is closed. (If the shared point is theoretically removed)
                     */
                    bool IsClosed();

                    /**
                     * @brief Removes a vertex
                     */
                    void Remove(const CVector &_idx);

                    /**
                     * @brief Optimizes a polygon.
                     */
                    void Optimize();

                    /**
                     * @return Returns the triangulated polygon.
                     */
                    std::list<Triangle> Triangulate();

                    ~CPolygon() = default;
                private:
                    struct SPoint
                    {
                        public:
                            SPoint(const CVector &_pos) : Position(_pos) {}

                            CVector Position;       //!< Position in 3d space.
                            CVector Index;          //!< Indices
                            Material Mat;           //!< Material

                            /**
                             * @brief Adds the trailing vertex 
                             */
                            void AddNext(SPoint* p);

                            /**
                             * @brief Adds the previous vertex 
                             */
                            void AddPrev(SPoint* p);

                            /**
                             * @brief Removes this vertex from all connected ones.
                             */
                            void Remove();

                            /**
                             * @brief Removes the given vertex from all lists.
                             */
                            void Remove(SPoint* p);

                            // Edge
                            std::map<CVector, SPoint*> nexts;
                            std::map<CVector, SPoint*> prevs;
                    };

                    CVector ProjectToCube(const CVector &_position);

                    CVector CalcCenter(Triangle t, const std::vector<CVector> &_verts);
                    bool IsClockwise(Triangle t, const std::vector<CVector> &_verts);

                    std::map<CVector, SPoint*> m_Points;
                    CVector m_Normal;
                    CObjectPool<SPoint> m_Pool;
            };

            std::vector<Point> m_Points;
            std::map<CVector, std::list<Triangle>, std::less<CVector>, CAllocator<CVector>> m_Triangles;
            std::vector<int> m_VerticesCounter;

            void GenerateTriangles(Mesh mesh);
            void GeneratePoints(Mesh mesh, const CVector &_curIdx, const std::list<Triangle> &_triangles);
    };
} // namespace VoxelOptimizer


#endif //VERTICESREDUCER_HPP