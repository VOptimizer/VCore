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

#ifndef VOXELANIMATION_HPP
#define VOXELANIMATION_HPP

#include <VCore/Voxel/VoxelModel.hpp>
#include <vector>

namespace VCore
{
    struct SVoxelFrame
    {
        SVoxelFrame(const VoxelModel &_Model, unsigned int _FrameTime) : Model(_Model), FrameTime(_FrameTime) {}

        VoxelModel Model;
        unsigned int FrameTime; //!< How long this frame should be last, in ms.
    };
    

    class CVoxelAnimation
    {
        public:
            static const int FRAME_TIME = 50;

            CVoxelAnimation() = default;

            /**
             * @brief Adds a new Frame to the collection
             * 
             * @param _Model: Model of the new frame.
             * @param _FrameTime: How long this frame should be last, in ms.
            */
            void AddFrame(VoxelModel _Model, unsigned int _FrameTime);

            /**
             * @brief Removes a frame from the animation.
             * 
             * @param _Frame: Frame index starts by 0.
             */
            void RemoveFrame(size_t _Frame);

            /**
             * @return Returns the frame count
             */
            inline size_t GetFrameCount() const
            {
                return m_Frames.size();
            }

            /**
             * @param _Frame: Frame index starts by 0.
             * 
             * @return Returns the frame data of a given frame.
             */
            SVoxelFrame GetFrame(size_t _Frame) const;

            ~CVoxelAnimation() = default;

        private:
            std::vector<SVoxelFrame> m_Frames;
    };

    using VoxelAnimation = std::shared_ptr<CVoxelAnimation>;
} // namespace VCore

#endif