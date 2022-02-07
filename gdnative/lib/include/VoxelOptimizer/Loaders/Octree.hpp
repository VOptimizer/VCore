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

#include <VoxelOptimizer/BBox.hpp>
#include <map>
#include <utility>
#include <vector>

#include <cmath>

#define f2i(x) (int)x//lfloor() //lroundf(x - 0.5f)

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
                size_t CalcIdxTime;
                size_t FindNodeTime;

                const int NODES_COUNT = 8;

                COctreeNode();
                COctreeNode(COctreeNode<T> *_parent, const CBBoxi &_bbox, int _depth); 
                COctreeNode(COctreeNode<T> *_parent, const COctreeNode<T> *_other); 

                virtual bool CanSubdivide();
                void Subdivide();
                COctreeNode<T> *FindNode(const CVectori &v);

                void CopyNode(const COctreeNode<T> *_other);
                void queryVisible(COctreeNode<T> *_root, std::map<CVectori, T> &_visible);

                void SetBBox(const CBBoxi &_bbox);

                size_t CalcIndex(const CVectori &_pos);
                bool HasNeighbor(const CVectori &_pos);

                bool m_HasContent;

                CVectori m_SubSize;
                CVector m_ReciprocalSubSize;

                // CBBox m_VoxelMeshSize;
                CBBoxi m_BBox;
                int m_MaxDepth;

                COctreeNode<T> **m_Nodes;
                COctreeNode<T> *m_Parent;

                std::map<CVectori, T> m_Content;
        };
    }

    template<class T>
    class COctreeIterator
    {
        using node_type = internal::COctreeNode<T>;
        using map_iterator = typename std::map<CVectori, T>::iterator;
        using reference = typename std::map<CVectori, T>::reference;
        using pointer = typename std::map<CVectori, T>::pointer;

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
        using pair = std::pair<CVectori, T>;
        using value_type = T;
        using iterator = COctreeIterator<T>;

        public:
            COctree();
            COctree(const COctree<T> &_tree);
            COctree(const CVectori &_size, int _depth = 10);

            std::map<CVectori, T> queryVisible();

            void insert(const pair &_pair);
            iterator erase(const iterator &_it) { return end(); }

            iterator find(const CVectori &_v);
            size_t size();

            iterator begin();
            iterator end();

            COctree<T> &operator=(const COctree<T> &_tree);

            virtual ~COctree() {}
        private:
            bool CanSubdivide() override;

            int RoundToPowerOfTwo(int v);
            CVectori RoundVectorPowerOfTwo(const CVectori &vec);

            size_t m_Size;
    };

    //////////////////////////////////////////////////
    // COctreeNode functions
    //////////////////////////////////////////////////

    namespace internal
    {
        template<class T>
        inline COctreeNode<T>::COctreeNode() : m_MaxDepth(0), m_Parent(nullptr), m_Nodes(nullptr), m_HasContent(false)
        {
            CalcIdxTime = 0;
            FindNodeTime = 0;
        }

        template<class T>
        inline COctreeNode<T>::COctreeNode(COctreeNode<T> *_parent, const CBBoxi &_bbox, int _depth) : COctreeNode()
        {
            m_MaxDepth = _depth;
            SetBBox(_bbox);
            m_Parent = _parent;
        }

        template<class T>
        inline COctreeNode<T>::COctreeNode(COctreeNode<T> *_parent, const COctreeNode<T> *_other) : COctreeNode()
        {
            m_Parent = _parent;
            CopyNode(_other);
        }

        template<class T>
        inline void COctreeNode<T>::CopyNode(const COctreeNode<T> *_other)
        {
            clear();

            SetBBox(_other->m_BBox);
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
        inline bool COctreeNode<T>::HasNeighbor(const CVectori &_pos)
        {
            return m_Content.find(_pos) != m_Content.end();
        }

        template<class T>
        inline void COctreeNode<T>::queryVisible(COctreeNode<T> *_root, std::map<CVectori, T> &_visible)
        {
            for (auto &&n : m_Content)
            {
                bool visible = false;

                // Goes through each axis
                for (char i = 0; i < 3; i++)
                {
                    // Goes from the front to the back of the cube
                    for (char j = -1; j <= 1; j += 2)
                    {
                        CVectori dir;
                        dir.v[i] = j;

                        CVectori pos = n.first + dir;

                        // Checks if the neighbor is inside this chunk
                        if(!m_BBox.ContainsPoint(pos))
                            visible = !_root->FindNode(pos)->HasNeighbor(pos);
                        else
                            visible = !HasNeighbor(pos);
                            
                        // When at least one side is visible, break the loops.
                        if(visible)
                        {
                            _visible.insert(n);
                            i = 5;
                            break;
                        }
                    }
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
            m_Nodes = new COctreeNode<T>*[NODES_COUNT];
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
                        CVectori position = m_BBox.Beg + m_SubSize * CVectori(x, y, z);
                        COctreeNode<T> *node = new COctreeNode<T>(this, CBBoxi(position, position + m_SubSize - CVectori(1, 1, 1)), m_MaxDepth - 1);
                        m_Nodes[idx] = node;
                        idx++;
                    }
                }
            }            
        }

        template<class T>
        inline bool COctreeNode<T>::CanSubdivide()
        {
            return m_MaxDepth != 0 && (m_SubSize.x > 8 && m_SubSize.y > 8 && m_SubSize.z > 8);
        }

        template<class T>
        inline COctreeNode<T> *COctreeNode<T>::FindNode(const CVectori &v)
        {
            COctreeNode<T> *ret = this;
            COctreeNode<T> **nodes = m_Nodes;

            while (nodes)
            {
                size_t idx = ret->CalcIndex(v);                
                if(idx >= NODES_COUNT)
                    return this;

                ret = nodes[idx];
                nodes = ret->m_Nodes;
            }
            
            return ret;
        }

        template<class T>
        inline void COctreeNode<T>::SetBBox(const CBBoxi &_bbox)
        {
            m_BBox = _bbox;
            m_SubSize = m_BBox.GetSize() * 0.5f;

            for (char i = 0; i < 3; i++)
            {
                if(m_SubSize.v[i] != 0.0)
                    m_ReciprocalSubSize.v[i] = 1.f / m_SubSize.v[i];
            }
        }

        template<class T>
        inline size_t COctreeNode<T>::CalcIndex(const CVectori &_pos)
        {
            // Position relative to the beginning of the cube.
            CVectori relPosition = (_pos - m_BBox.Beg);

            // Snaps the position to the grid.
            CVectori vidx = relPosition / m_SubSize; //* m_ReciprocalSubSize;

            // Calculates the index of the subcube.
            return vidx.x + 2 * vidx.y + 4 * vidx.z;
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
    inline COctree<T>::COctree(const CVectori &_size, int _depth) : internal::COctreeNode<T>(nullptr, CBBoxi(), _depth)
    {
        // Creates a voxel space that is a power of two in size.
        this->SetBBox(CBBoxi(CVectori(), RoundVectorPowerOfTwo(_size) - CVectori(1, 1, 1)));
        m_Size = 0;
    }

    template<class T>
    inline void COctree<T>::insert(const COctree<T>::pair &_pair)
    {
        m_Size++;
        internal::COctreeNode<T> *node = this->FindNode(_pair.first);
        node->m_HasContent = true;

        /**
         * Subdivides each cube
         */
        while (node->CanSubdivide())
        {
            node->Subdivide();
            node = node->FindNode(_pair.first);
            node->m_HasContent = true;
        }
        
        node->m_Content.insert(_pair);
    }

    template<class T>
    inline size_t COctree<T>::size()
    {
        return m_Size;
    }

    template<class T>
    inline std::map<CVectori, T> COctree<T>::queryVisible()
    {
        std::map<CVectori, T> ret;

        if(this->m_Nodes)
        {
            internal::COctreeNode<T> *node = this->m_Nodes[0];
            while(node->CanSubdivide())
                node = node->m_Nodes[0];

            while (node != this)
            {
                while(node->m_Content.empty())
                {
                    internal::COctreeNode<T> *parent = node->m_Parent;
                    if(!parent)
                        break;

                    size_t idx = parent->CalcIndex(node->m_BBox.Beg);
                    if(idx < (this->NODES_COUNT - 1))
                    {
                        idx++;
                        node = parent->m_Nodes[idx];   

                        while(node->CanSubdivide() && node->m_Nodes)
                            node = node->m_Nodes[0];                
                    }
                    else
                        node = parent;
                }

                if(node != this)
                {
                    node->queryVisible(this, ret);

                    size_t idx = node->m_Parent->CalcIndex(node->m_BBox.Beg);
                    if(idx < (this->NODES_COUNT - 1))
                    {
                        idx++;
                        node = node->m_Parent->m_Nodes[idx];   
                    }
                    else
                        node = node->m_Parent;
                }
            }
        }

        return ret;
    }

    template<class T>
    inline typename COctree<T>::iterator COctree<T>::find(const CVectori &_v)
    {
        size_t idx = this->CalcIndex(_v);
        if(idx < this->NODES_COUNT && this->m_Nodes[idx]->m_HasContent)
        {
            internal::COctreeNode<T> *node  = this->FindNode(_v);  
            typename std::map<CVectori, T>::iterator current = node->m_Content.find(_v);
            if(current != node->m_Content.end())
                return iterator(node, current);
        }

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

            size_t idx = parent->CalcIndex(node->m_BBox.Beg);
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

    template<class T>
    inline bool COctree<T>::CanSubdivide()
    {
        return true;
    }

    /**
     * @return Returns a Vector rounded to the nearest power of two.
     */
    template<class T>
    inline CVectori COctree<T>::RoundVectorPowerOfTwo(const CVectori &vec)
    {
        CVectori ret;

        ret.x = RoundToPowerOfTwo(vec.x);
        ret.y = RoundToPowerOfTwo(vec.y);
        ret.z = RoundToPowerOfTwo(vec.z);

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

                size_t idx = parent->CalcIndex(m_Node->m_BBox.Beg);
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