/*
 * GLGeometryShader.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_GL_GEOMETRY_SHADER_H__
#define __LLGL_GL_GEOMETRY_SHADER_H__


#include <LLGL/GeometryShader.h>
#include "GLHardwareShader.h"


namespace LLGL
{


class GLGeometryShader : public GeometryShader
{

    public:

        GLGeometryShader();

        bool Compile(const std::string& shaderSource) override;

        std::string QueryInfoLog() override;

        GLHardwareShader hwShader;

};


} // /namespace LLGL


#endif



// ================================================================================