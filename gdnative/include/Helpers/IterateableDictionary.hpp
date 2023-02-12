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

#ifndef ITERATEABLEDICTIONARY_HPP
#define ITERATEABLEDICTIONARY_HPP

#include <Dictionary.hpp>
#include <utility>

class CDictionaryIterator
{
    public:
        CDictionaryIterator(godot::Dictionary &_Dictionary, size_t _Index = 0) : m_Dictionary(_Dictionary), m_Keys(_Dictionary.keys()), m_Index(_Index) {}

        CDictionaryIterator &operator++();
        bool operator!=(const CDictionaryIterator &_Other) const;

        std::pair<godot::Variant, godot::Variant> &operator*() const;
        std::pair<godot::Variant, godot::Variant> *operator->() const;

        ~CDictionaryIterator() = default;

    private:
        godot::Dictionary &m_Dictionary;
        godot::Array m_Keys;
        size_t m_Index;
        std::pair<godot::Variant, godot::Variant> m_CurrentPair;
};

// std::pair<godot::Variant, godot::Variant>

class CIterateableDictionary
{
    public:
        CIterateableDictionary(godot::Dictionary &_Dictionary) : m_Dictionary(_Dictionary) {}

        const CDictionaryIterator begin() const;
        const CDictionaryIterator end() const;

        ~CIterateableDictionary() = default;
    private:
        godot::Dictionary &m_Dictionary;
};

//////////////////////////////////////////////////
// CDictionaryIterator functions
//////////////////////////////////////////////////

inline CDictionaryIterator &CDictionaryIterator::operator++()
{
    m_Index++;

    if(m_Index < m_Dictionary.size())
    {
        auto key = m_Keys[m_Index];
        m_CurrentPair = { key, m_Dictionary[key] };
    }

    return *this;
}

inline bool CDictionaryIterator::operator!=(const CDictionaryIterator &_Other) const
{
    return m_Index != _Other.m_Index;
}

inline std::pair<godot::Variant, godot::Variant> &CDictionaryIterator::operator*() const
{
    return m_CurrentPair;
}

inline std::pair<godot::Variant, godot::Variant> *CDictionaryIterator::operator->() const
{
    return &m_CurrentPair;
}

//////////////////////////////////////////////////
// CIterateableDictionary functions
//////////////////////////////////////////////////

inline const CDictionaryIterator CIterateableDictionary::begin() const
{
    return CDictionaryIterator(m_Dictionary);
}

inline const CDictionaryIterator CIterateableDictionary::end() const
{
    return CDictionaryIterator(m_Dictionary, m_Dictionary.size());
}

#endif