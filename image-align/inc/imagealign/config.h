/**
 This file is part of Image Alignment.
 
 Copyright Christoph Heindl 2015
 
 Image Alignment is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Image Alignment is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Image Alignment.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IMAGE_ALIGN_CONFIG_H
#define IMAGE_ALIGN_CONFIG_H

#include <opencv2/core/version.hpp>

#ifdef _WIN32

    #define IA_DISABLE_PRAGMA_WARN(nnn) \
    __pragma (warning (push)) \
    __pragma (warning(disable : nnn)) \

    #define IA_DISABLE_PRAGMA_WARN_END \
    __pragma (warning (pop)) \

#else

    #define IA_DISABLE_PRAGMA_WARN(nnn)
    #define IA_DISABLE_PRAGMA_WARN_END

#endif

#if CV_MAJOR_VERSION == 2
    #define IA_CV_VERSION 2
#elif CV_MAJOR_VERSION == 3
    #define IA_CV_VERSION 3
#endif


#endif