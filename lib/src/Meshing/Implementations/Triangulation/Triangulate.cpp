/*
* MIT License
*
* Copyright (c) 2022 Christian Tost
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

#include "Triangulate.hpp"

namespace VoxelOptimizer
{
    static const float EPSILON = 0.0000000001f;

    bool CTriangulate::Triangulate(const std::vector<CVector> &_polygon, std::vector<int> &_indices)
    {
        /* allocate and initialize list of Vertices in polygon */
        int n = _polygon.size();
        if (n < 3) 
            return false;

        int *V = new int[n];

        /* we want a counter-clockwise polygon in V */
        if (0.0f < Area(_polygon))
        {
            for (int v = 0; v < n; v++) 
                V[v] = v;
        }
        else
        {
            for(int v = 0; v < n; v++) 
                V[v] = (n - 1) - v;
        }

        int nv = n;

        /*  remove nv-2 Vertices, creating 1 triangle every time */
        int count = 2 * nv;   /* error detection */
        for(int m = 0, v = nv - 1; nv > 2;)
        {
            /* if we loop, it is probably a non-simple polygon */
            if (0 >= (count--))
                return false; //** Triangulate: ERROR - probable bad polygon!

            /* three consecutive vertices in current polygon, <u,v,w> */
            int u = v; 
            if (nv <= u) 
                u = 0;     /* previous */

            v = u+1; 
            if (nv <= v) 
                v = 0;     /* new v    */

            int w = v+1; 
            if (nv <= w) 
                w = 0;     /* next     */

            if (Snip(_polygon, u, v, w, nv, V))
            {
                int a, b, c, s, t;

                /* true names of the vertices */
                a = V[u]; 
                b = V[v]; 
                c = V[w];

                /* output Triangle */
                _indices.push_back(a);
                _indices.push_back(b);
                _indices.push_back(c);

                m++;

                /* remove v from remaining polygon */
                for(s = v, t = v + 1; t < nv; s++, t++) 
                    V[s] = V[t]; 

                nv--;

                /* resest error detection counter */
                count = 2 * nv;
            }
        }

        delete V;
        return true;
    }

    float CTriangulate::Area(const std::vector<CVector> &_polygon)
    {
        int n = _polygon.size();

        float A = 0.0f;
        for(int p = n - 1, q = 0; q < n; p = q++)
            A += _polygon[p].x * _polygon[q].y - _polygon[q].x * _polygon[p].y;

        return A * 0.5f;
    }

    bool CTriangulate::PointInTriangle(const CVector &_p, const CVector &_a, const CVector &_b, const CVector &_c)
    {
        float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
        float cCROSSap, bCROSScp, aCROSSbp;

        ax = _c.x - _b.x;  ay = _c.y - _b.y;
        bx = _a.x - _c.x;  by = _a.y - _c.y;
        cx = _b.x - _a.x;  cy = _b.y - _a.y;
        apx= _p.x - _a.x;  apy= _p.y - _a.y;
        bpx= _p.x - _b.x;  bpy= _p.y - _b.y;
        cpx= _p.x - _c.x;  cpy= _p.y - _c.y;

        aCROSSbp = ax*bpy - ay*bpx;
        cCROSSap = cx*apy - cy*apx;
        bCROSScp = bx*cpy - by*cpx;

        return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));

        // float d1 = Cross2D(_p - _a, _b - _a);
        // float d2 = Cross2D(_p - _b, _c - _b);
        // float d3 = Cross2D(_p - _c, _a - _c);

        // return (d1 >= 0 && d2 >= 0 && d3 >= 0) || (d1 <= 0 && d2 <= 0 && d3 <= 0);
    }

    bool CTriangulate::Snip(const std::vector<CVector> &_polygon,int u,int v,int w,int n,int *V)
    {
        const CVector &a = _polygon[V[u]];
        const CVector &b = _polygon[V[v]];
        const CVector &c = _polygon[V[w]];

        if (EPSILON > (((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x)))) 
            return false;

        for (int p = 0; p < n; p++)
        {
            if((p == u) || (p == v) || (p == w)) 
                continue;

            const CVector &point = _polygon[V[p]];

            if (PointInTriangle(point, a, b, c)) 
                return false;
        }

        return true;
    }

    float CTriangulate::Cross2D(const CVector &_a, const CVector &_b)
    {
        return _a.x * _b.y - _a.y * _b.x;
    }
} // namespace VoxelOptimizer