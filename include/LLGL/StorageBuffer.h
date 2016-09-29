/*
 * StorageBuffer.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_STORAGE_BUFFER_H__
#define __LLGL_STORAGE_BUFFER_H__


#include "Export.h"
#include "RenderSystemFlags.h"
#include "BufferFlags.h"
#include <string>


namespace LLGL
{


//! Storage buffer descriptor structure.
struct StorageBufferDescriptor
{
    StorageBufferDescriptor() = default;

    StorageBufferDescriptor(unsigned int size, BufferUsage usage) :
        size    ( size  ),
        usage   ( usage )
    {
    }

    //! Buffer size (in bytes).
    unsigned int        size    = 0;

    //! Buffer usage (typically "BufferUsage::Dynamic", since a storage buffer is commonly frequently changed).
    BufferUsage         usage   = BufferUsage::Dynamic;

    /**
    \brief Specifies the storage buffer type.
    \remarks In OpenGL there are only generic storage buffers (or rather "Shader Storage Buffer Objects").
    \note Only supported with: Direct3D 11, Direct3D 12.
    */
    StorageBufferType   type    = StorageBufferType::Buffer;
};

/**
\brief Storage buffer shader-view descriptor structure.
\remarks This structure is used to describe the view of a storage buffer within a shader.
*/
struct StorageBufferViewDescriptor
{
    std::string         name;           //!< Storage buffer name.
    unsigned int        index   = 0;    //!< Index of the storage buffer within the respective shader.

    /**
    \brief Storage buffer type.
    \remarks For the OpenGL render system, this type is always 'StorageBufferType::Buffer',
    since GLSL only supports generic shader storage buffers. Here is an example:
    \code
    layout(std430, binding=0) buffer myBuffer
    {
        vec4 myBufferArray[];
    };
    \endcode
    \note Only supported with: Direct3D 11, Direct3D 12
    */
    StorageBufferType   type    = StorageBufferType::Buffer;
};


//! Storage buffer (also called "Shader Storage Buffer Object" or "Read/Write Buffer") interface.
class LLGL_EXPORT StorageBuffer
{

    public:

        virtual ~StorageBuffer()
        {
        }

};


} // /namespace LLGL


#endif



// ================================================================================
