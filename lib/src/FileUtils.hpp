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

#ifndef FILEUTILS_HPP
#define FILEUTILS_HPP

#include <algorithm>
#include <string>
#include <utility>

namespace VCore
{
    inline std::string GetFileExt(std::string p_Path)
    {
        // Removes the file name.
        size_t Pos = p_Path.find_last_of(".");
        if(Pos != std::string::npos)
            p_Path = p_Path.erase(0, Pos + 1);

        std::transform(p_Path.begin(), p_Path.end(), p_Path.begin(), ::tolower);

        return p_Path;
    }

    inline std::string GetBasename(std::string p_Path)
    {
        // Replaces all '\' to '/'
        std::replace(p_Path.begin(), p_Path.end(), '\\', '/');

        size_t Pos = p_Path.find_last_of("/");
        return p_Path.substr(0, Pos);
    }

    inline std::string GetPathWithoutExt(std::string p_Path)
    {
        // Removes the file extension.
        size_t Pos = p_Path.find_last_of(".");
        if(Pos != std::string::npos)
            p_Path = p_Path.erase(Pos);

        return p_Path;
    }

    inline std::string GetFilename(std::string p_Path)
    {
        // Replaces all '\' to '/'
        std::replace(p_Path.begin(), p_Path.end(), '\\', '/');

        // Deletes the path.
        size_t Pos = p_Path.find_last_of("/");
        if(Pos != std::string::npos)
            p_Path = p_Path.substr(Pos + 1);

        return p_Path;
    }

    inline std::string GetFilenameWithoutExt(std::string p_Path)
    {
        return GetFilename(GetPathWithoutExt(std::move(p_Path)));
    }
}

#endif //FILEUTILS_HPP