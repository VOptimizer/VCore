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
                    CTriangle() : Processed(false) {}

                    std::vector<CVector> Indices;
                    Material Mat;
                    bool Processed;
            };
            using Triangle = std::shared_ptr<CTriangle>;

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
                            float Angle;

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
                    CVector m_Center;
            };

            std::vector<Point> m_Points;
            std::map<CVector, std::list<Triangle>, std::less<CVector>, CAllocator<CVector>> m_Triangles;
            std::vector<int> m_VerticesCounter;

            void GenerateTriangles(Mesh mesh);
            void GeneratePoints(Mesh mesh, const CVector &_curIdx, const std::list<Triangle> &_triangles);
    };
} // namespace VoxelOptimizer


#endif //VERTICESREDUCER_HPP