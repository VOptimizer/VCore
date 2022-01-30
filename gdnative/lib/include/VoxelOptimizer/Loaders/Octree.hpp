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

#ifndef OCTREE_HPP
#define OCTREE_HPP

#include <chrono>

#include <VoxelOptimizer/BBox.hpp>
#include <map>
#include <utility>
#include <vector>

namespace VoxelOptimizer
{
    template<class T>
    class COctree;

    template<class T>
    class COctreeIterator;

    namespace internal
    {
        template<class T>
        class COctreeNode
        {
            friend COctree<T>;
            friend COctreeIterator<T>;
            public:
                void clear();

                virtual ~COctreeNode();
            protected:
                const int NODES_COUNT = 8;

                COctreeNode();
                COctreeNode(COctreeNode<T> *_parent, const CBBox &_bbox, int _depth); 
                COctreeNode(COctreeNode<T> *_parent, const COctreeNode<T> *_other); 

                bool CanSubdivide();
                void Subdivide();
                COctreeNode<T> *FindNode(const CVector &v);

                void CopyNode(const COctreeNode<T> *_other);

                CBBox m_BBox;
                int m_MaxDepth;

                COctreeNode<T> **m_Nodes;
                COctreeNode<T> *m_Parent;

                std::map<CVector, T> m_Content;
        };

        size_t CalcIndex(const CBBox &_bbox, const CVector &_pos);
    }

    template<class T>
    class COctreeIterator
    {
        using node_type = internal::COctreeNode<T>;
        using map_iterator = typename std::map<CVector, T>::iterator;
        using reference = typename std::map<CVector, T>::reference;
        using pointer = typename std::map<CVector, T>::pointer;

        public:
            COctreeIterator(node_type *_node);
            COctreeIterator(node_type *_node, map_iterator _current);

            reference operator*() const;
            pointer operator->() const;

            COctreeIterator<T>& operator++();
		    COctreeIterator<T>& operator--();

            bool operator!=(const COctreeIterator<T> &b);
            bool operator==(const COctreeIterator<T> &b);

        private:
            void SetNode(node_type *_node);

            node_type *m_Node;
            map_iterator m_Current;
            map_iterator m_Begin;
            map_iterator m_End;
    };

    /**
     * An octree allows us to travel faster and more efficiently through the voxel data.
     * The generic type allows us to store any data in a 3D location.
     * 
     * The class is designed to fit perfect in the stl standard.
     */
    template<class T>
    class COctree : public internal::COctreeNode<T>
    {
        using pair = std::pair<CVector, T>;
        using value_type = T;
        using iterator = COctreeIterator<T>;

        public:
            COctree();
            COctree(const COctree<T> &_tree);
            COctree(const CVector &_size, int _depth = 10);

            void insert(const pair &_pair);
            iterator erase(const iterator &_it) {}

            iterator find(const CVector &_v);
            size_t size();

            iterator begin();
            iterator end();

            COctree<T> &operator=(const COctree<T> &_tree);

            virtual ~COctree() {}
        private:
            int RoundToPowerOfTwo(int v);
            CVector RoundVectorPowerOfTwo(const CVector &vec);

            size_t m_Size;
    };

    //////////////////////////////////////////////////
    // COctreeNode functions
    //////////////////////////////////////////////////

    namespace internal
    {
        template<class T>
        inline COctreeNode<T>::COctreeNode() : m_MaxDepth(0), m_Parent(nullptr), m_Nodes(nullptr)
        {

        }

        template<class T>
        inline COctreeNode<T>::COctreeNode(COctreeNode<T> *_parent, const CBBox &_bbox, int _depth) : m_Nodes(nullptr)
        {
            m_MaxDepth = _depth;
            m_BBox = _bbox;
            m_Parent = _parent;
        }

        template<class T>
        inline COctreeNode<T>::COctreeNode(COctreeNode<T> *_parent, const COctreeNode<T> *_other) : m_Nodes(nullptr)
        {
            m_Parent = _parent;
            CopyNode(_other);
        }

        template<class T>
        inline void COctreeNode<T>::CopyNode(const COctreeNode<T> *_other)
        {
            clear();

            m_BBox = _other->m_BBox;
            m_MaxDepth = _other->m_MaxDepth;
            m_Content = _other->m_Content;

            if(_other->m_Nodes)
            {
                m_Nodes = new COctreeNode<T>*[NODES_COUNT];
                size_t idx = 0;

                for (int i = 0; i < NODES_COUNT; i++)
                {
                    m_Nodes[idx] = new COctreeNode<T>(this, _other->m_Nodes[i]);
                    idx++;
                }
            }
        }

        template<class T>
        inline void COctreeNode<T>::clear()
        {
            if(!m_Nodes)
                return;

            for (int i = 0; i < NODES_COUNT; i++)
                delete m_Nodes[i];

            delete[] m_Nodes;
            m_Nodes = nullptr;
        }

        template<class T>
        inline COctreeNode<T>::~COctreeNode()
        {
            clear();
        }

        template<class T>
        inline void COctreeNode<T>::Subdivide()
        {
            if(m_MaxDepth == 0)
                return;

            if(m_Nodes)
                throw std::runtime_error("Already subdivided!");

            m_Nodes = new COctreeNode<T>*[NODES_COUNT];
            CVector subSize = m_BBox.GetSize() * 0.5f;
            size_t idx = 0;

            /**
             * Subdivision starts at the bottom left front and goes to top right front.
             * This is then repeated in the z-direction.
             */
            for (char z = 0; z < 2; z++)
            {
                for (char y = 0; y < 2; y++)
                {
                    for (char x = 0; x < 2; x++)
                    {
                        CVector position = m_BBox.Beg + subSize * CVector(x, y, z);
                        COctreeNode<T> *node = new COctreeNode<T>(this, CBBox(position, position + subSize), m_MaxDepth - 1);
                        m_Nodes[idx] = node;
                        idx++;
                    }
                }
            }            
        }

        template<class T>
        inline bool COctreeNode<T>::CanSubdivide()
        {
            return m_MaxDepth != 0;
        }

        template<class T>
        inline COctreeNode<T> *COctreeNode<T>::FindNode(const CVector &v)
        {
            if(!m_Nodes)
                return this;

            COctreeNode<T> *ret = this;
            COctreeNode<T> **nodes = m_Nodes;

            while (nodes)
            {
                size_t idx = CalcIndex(ret->m_BBox, v);
                if(idx >= NODES_COUNT)
                    return this;

                ret = nodes[idx];
                nodes = ret->m_Nodes;
            }
            
            return ret;

            // // Calculates the index of the subcube.
            // size_t idx = CalcIndex(m_BBox, v);
            // if(idx >= m_Nodes.size())
            //     return this;

            // // Finds the next node in the subcube.
            // return m_Nodes[idx]->FindNode(v);
        }

        inline size_t CalcIndex(const CBBox &_bbox, const CVector &_pos)
        {
            // Position relative to the beginning of the cube.
            CVector relPosition = (_pos - _bbox.Beg);

            // Snaps the position to the grid.
            CVector vidx = (relPosition / (_bbox.GetSize() * 0.5)); //.Floor();

            // Calculates the index of the subcube.
            return int(vidx.x) + 2 * int(vidx.y) + 4 * int(vidx.z);
        }
    } // namespace internal

    //////////////////////////////////////////////////
    // COctree functions
    //////////////////////////////////////////////////

    template<class T>
    inline COctree<T>::COctree() : internal::COctreeNode<T>()
    {
        m_Size = 0;
    }

    template<class T>
    inline COctree<T>::COctree(const COctree<T> &_tree) : internal::COctreeNode<T>(nullptr, &_tree)
    {
        m_Size = _tree.m_Size;
    }

    template<class T>
    inline COctree<T>::COctree(const CVector &_size, int _depth) : internal::COctreeNode<T>(nullptr, CBBox(), _depth)
    {
        // Creates a voxel space that is a power of two in size.
        this->m_BBox = CBBox(CVector(), RoundVectorPowerOfTwo(_size) - CVector(1, 1, 1));
        m_Size = 0;
    }

    template<class T>
    inline void COctree<T>::insert(const COctree<T>::pair &_pair)
    {
        m_Size++;
        std::chrono::steady_clock::time_point begin1 = std::chrono::steady_clock::now();
        internal::COctreeNode<T> *node = this->FindNode(_pair.first);
        std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();
        auto count1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

        if(node->CanSubdivide() && node->m_Nodes)
            throw std::runtime_error("BUG!!!");

        while (node->CanSubdivide())
        {
            node->Subdivide();
            node = node->FindNode(_pair.first);
        }
        
        node->m_Content.insert(_pair);
    }

    template<class T>
    inline size_t COctree<T>::size()
    {
        return m_Size;
    }

    template<class T>
    inline typename COctree<T>::iterator COctree<T>::find(const CVector &_v)
    {
        std::chrono::steady_clock::time_point begin1 = std::chrono::steady_clock::now();
        internal::COctreeNode<T> *node = this->FindNode(_v);
        std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();
        auto count1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

        count1 = 0;

        if(node->m_Content.size() > 100)
        {
            int kk = 0;
            kk++;
        }

        
        typename std::map<CVector, T>::iterator current = node->m_Content.find(_v);

        if(current != node->m_Content.end())
            return iterator(node, current);

        return end();
    }

    template<class T>
    inline typename COctree<T>::iterator COctree<T>::begin()
    {
        if(!this->m_Nodes)
            return end();

        internal::COctreeNode<T> *node = this->m_Nodes[0];
        while(node->CanSubdivide())
            node = node->m_Nodes[0];

        while(node->m_Content.empty())
        {
            internal::COctreeNode<T> *parent = node->m_Parent;
            if(!parent)
                break;

            size_t idx = internal::CalcIndex(parent->m_BBox, node->m_BBox.Beg);
            if(idx < (this->NODES_COUNT - 1))
            {
                idx++;
                node = parent->m_Nodes[idx];   

                while(node->CanSubdivide())
                    node = node->m_Nodes[0];                
            }
            else
                node = parent;
        }

        return iterator(node);
    }

    template<class T>
    inline typename COctree<T>::iterator COctree<T>::end()
    {
        return iterator(this);
    }

    template<class T>
    inline COctree<T> &COctree<T>::operator=(const COctree<T> &_tree)
    {
        m_Size = _tree.m_Size;
        this->CopyNode(&_tree);

        return *this;
    }

    /**
     * @brief Rounds to the next power of 2 size.
     */
    template<class T>
    inline int COctree<T>::RoundToPowerOfTwo(int v)
    {
        //Source: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 (24.01.2022)
        if(v != 0)
        {
            v--;
            v |= (v >> 1);
            v |= (v >> 2);
            v |= (v >> 4);
            v |= (v >> 8);
            v |= (v >> 16);
            v++;
        }

        return v;
    }

    /**
     * @return Returns a Vector rounded to the nearest power of two.
     */
    template<class T>
    inline CVector COctree<T>::RoundVectorPowerOfTwo(const CVector &vec)
    {
        CVector ret;

        ret.x = RoundToPowerOfTwo((int)vec.x);
        ret.y = RoundToPowerOfTwo((int)vec.y);
        ret.z = RoundToPowerOfTwo((int)vec.z);

        return ret;
    }

    //////////////////////////////////////////////////
    // COctreeIterator functions
    //////////////////////////////////////////////////

    template<class T>
    inline COctreeIterator<T>::COctreeIterator(COctreeIterator<T>::node_type *_node)
    {
        SetNode(_node);
    }

    template<class T>
    inline COctreeIterator<T>::COctreeIterator(COctreeIterator<T>::node_type *_node, COctreeIterator<T>::map_iterator _current) : COctreeIterator(_node)
    {
        m_Current = _current;
    }

    template<class T>
    inline typename COctreeIterator<T>::reference COctreeIterator<T>::operator*() const
    {
        return *m_Current;
    }

    template<class T>
    inline typename COctreeIterator<T>::pointer COctreeIterator<T>::operator->() const
    {
        return &(*m_Current);
    }

    template<class T>
    inline COctreeIterator<T>& COctreeIterator<T>::operator++()
    {
        m_Current++;

        if(m_Current == m_End)
        {
            while(true)
            {
                internal::COctreeNode<T> *parent = m_Node->m_Parent;
                if(!parent)
                    break;

                size_t idx = internal::CalcIndex(parent->m_BBox, m_Node->m_BBox.Beg);
                if(idx < (8 - 1))
                {
                    idx++;
                    SetNode(parent->m_Nodes[idx]);

                    while(m_Node->CanSubdivide() && !m_Node->m_Nodes)
                        SetNode(m_Node->m_Nodes[0]);

                    if(!m_Node->m_Content.empty())
                        break;                       
                }
                else
                    SetNode(parent);
            }
        }

        return *this;
    }

    template<class T>
    inline COctreeIterator<T>& COctreeIterator<T>::operator--()
    {
        if(m_Current != m_Begin)
            m_Current--;

        return *this;
    }

    template<class T>
    inline void COctreeIterator<T>::SetNode(node_type *_node)
    {
        m_Node = _node;
        m_Begin = m_Current = m_Node->m_Content.begin();
        m_End = m_Node->m_Content.end();
    }

    template<class T>
    inline bool COctreeIterator<T>::operator!=(const COctreeIterator<T> &b)
    {
        return m_Node != b.m_Node;
    }

    template<class T>
    inline bool COctreeIterator<T>::operator==(const COctreeIterator<T> &b)
    {
        return m_Node == b.m_Node;
    }
} // namespace VoxelOptimizer

#endif //OCTREE_HPP