/*
 * MIT License
 *
 * Copyright (c) 2025 Christian Tost
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

#ifndef MESSAGEBUS_HPP
#define MESSAGEBUS_HPP

#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <VCore/Voxel/BBox.hpp>
#include <cstdint>
#include <memory>

namespace VCore 
{
    template <class Key, class T>
    class IMessageHandler
    {
        public:
            /**
             * Called everytime a message for this handler was publish.
             */
            virtual void OnMessage(const Key&, const T&) = 0;
    };

    template <class Key, class T>
    class TMessageBus
    {
        public:
            TMessageBus() = default;
            TMessageBus(TMessageBus&&) = default;
            TMessageBus(const TMessageBus&) = default;

            TMessageBus &operator=(TMessageBus&&) = default;
            TMessageBus &operator=(const TMessageBus&) = default;

            static std::unique_ptr<TMessageBus<Key, T>> &GetInstance()
            {
                if(!m_Instance)
                    m_Instance = std::make_unique<TMessageBus<Key, T>>();

                return m_Instance;
            }

            /**
             * Adds a new handler.
             * @param p_Key: Key to listen on.
             * @param p_Handler: The handler itself
             */
            void AddHandler(Key p_Key, IMessageHandler<Key, T> *p_Handler)
            {
                m_Subscriptions[p_Key].push_back(p_Handler);
            }

            /**
             * Removes a handler.
             * @param p_Key: Key to listen on.
             * @param p_Handler: The handler itself
             */
            void RemoveHandler(Key p_Key, IMessageHandler<Key, T> *p_Handler)
            {
                auto it = m_Subscriptions.find(p_Key);
                if(it != m_Subscriptions.end())
                    it->second.remove(p_Handler);
            }

            /** Informs all handlers, which are listining for the key. */
            void PublishMessage(Key p_Key, const T &p_Message)
            {
                auto it = m_Subscriptions.find(p_Key);
                if(it != m_Subscriptions.end())
                {
                    for (auto &&handler : it->second) 
                        handler->OnMessage(p_Key, p_Message);
                }
            }

            ~TMessageBus() = default;
        private:
            static std::unique_ptr<TMessageBus<Key, T>> m_Instance;

            ankerl::unordered_dense::map<Key, fast_vector<IMessageHandler<Key, T>*>> m_Subscriptions;
    };

    template <class Key, class T>
    inline std::unique_ptr<TMessageBus<Key, T>> TMessageBus<Key, T>::m_Instance;

    using ModelBBoxMessageBus = TMessageBus<uintptr_t, CBBox>;
}

#endif