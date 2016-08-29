/*
 * RenderTarget.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include <LLGL/RenderTarget.h>
#include <Gauss/Equals.h>
#include <stdexcept>
#include <string>


namespace LLGL
{


RenderTarget::~RenderTarget()
{
}


/*
 * ======= Protected: =======
 */

void RenderTarget::ApplyResolution(const Gs::Vector2i& resolution)
{
    /* Validate texture attachment size */
    if (resolution.x == 0 || resolution.y == 0)
    {
        throw std::invalid_argument(
            "attachment to render target failed, due to invalid size (" +
            std::to_string(resolution.x) + ", " + std::to_string(resolution.x) + ")"
        );
    }

    /* Check if size matches the current resolution */
    if (resolution_ != Gs::Vector2i(0, 0))
    {
        if (resolution != resolution_)
            throw std::invalid_argument("attachment to to render target failed, due to resolution mismatch");
    }
    else
        resolution_ = resolution;
}

void RenderTarget::ResetResolution()
{
    resolution_ = Gs::Vector2i(0, 0);
}


} // /namespace LLGL



// ================================================================================