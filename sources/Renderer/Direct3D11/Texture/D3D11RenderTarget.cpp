/*
 * D3D11RenderTarget.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "D3D11RenderTarget.h"
#include "../D3D11RenderSystem.h"
#include "../../DXCommon/DXCore.h"
#include "../../CheckedCast.h"
#include "../../../Core/Helper.h"


namespace LLGL
{


D3D11RenderTarget::D3D11RenderTarget(ID3D11Device* device, const RenderTargetDescriptor& desc) :
    device_       { device                           },
    resolution_   { desc.resolution                  },
    multiSamples_ { desc.multiSampling.SampleCount() },
    renderPass_   { desc.renderPass                  }
{
    #if 0
    if (desc.attachments.empty())
    {
        //TODO...
    }
    else
    #endif
    {
        /* Initialize all attachments */
        for (const auto& attachment : desc.attachments)
            Attach(attachment);
    }
}

Extent2D D3D11RenderTarget::GetResolution() const
{
    return resolution_;
}

std::uint32_t D3D11RenderTarget::GetNumColorAttachments() const
{
    return static_cast<std::uint32_t>(renderTargetViews_.size());
}

bool D3D11RenderTarget::HasDepthAttachment() const
{
    return (depthStencilView_.Get() != nullptr);
}

bool D3D11RenderTarget::HasStencilAttachment() const
{
    return (depthStencilView_.Get() != nullptr && depthStencilFormat_ == DXGI_FORMAT_D24_UNORM_S8_UINT);
}

const RenderPass* D3D11RenderTarget::GetRenderPass() const
{
    return renderPass_;
}

/* ----- Extended Internal Functions ----- */

void D3D11RenderTarget::ResolveSubresources(ID3D11DeviceContext* context)
{
    for (const auto& attachment : multiSampledAttachments_)
    {
        context->ResolveSubresource(
            attachment.targetTexture,
            attachment.targetSubresourceIndex,
            attachment.texture2DMS.Get(),
            0,
            attachment.format
        );
    }
}


/*
 * ======= Private: =======
 */

void D3D11RenderTarget::Attach(const AttachmentDescriptor& attachmentDesc)
{
    if (auto texture = attachmentDesc.texture)
    {
        /* Attach texture */
        AttachTexture(*texture, attachmentDesc);
    }
    else
    {
        /* Attach (and create) depth-stencil buffer */
        switch (attachmentDesc.type)
        {
            case AttachmentType::Color:
                throw std::invalid_argument("cannot have color attachment in render target without a valid texture");
                break;
            case AttachmentType::Depth:
                AttachDepthBuffer();
                break;
            case AttachmentType::DepthStencil:
                AttachDepthStencilBuffer();
                break;
            case AttachmentType::Stencil:
                AttachStencilBuffer();
                break;
        }
    }
}

void D3D11RenderTarget::AttachDepthBuffer()
{
    CreateDepthStencilAndDSV(DXGI_FORMAT_D32_FLOAT);
}

void D3D11RenderTarget::AttachStencilBuffer()
{
    CreateDepthStencilAndDSV(DXGI_FORMAT_D24_UNORM_S8_UINT);
}

void D3D11RenderTarget::AttachDepthStencilBuffer()
{
    CreateDepthStencilAndDSV(DXGI_FORMAT_D24_UNORM_S8_UINT);
}

static void FillViewDescForTexture1D(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE1D;
    viewDesc.Texture1D.MipSlice = attachmentDesc.mipLevel;
}

static void FillViewDescForTexture2D(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipSlice = attachmentDesc.mipLevel;
}

static void FillViewDescForTexture3D(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension          = D3D11_RTV_DIMENSION_TEXTURE3D;
    viewDesc.Texture3D.MipSlice     = attachmentDesc.mipLevel;
    viewDesc.Texture3D.FirstWSlice  = attachmentDesc.arrayLayer;
    viewDesc.Texture3D.WSize        = 1;
}

static void FillViewDescForTexture1DArray(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
    viewDesc.Texture1DArray.MipSlice        = attachmentDesc.mipLevel;
    viewDesc.Texture1DArray.FirstArraySlice = attachmentDesc.arrayLayer;
    viewDesc.Texture1DArray.ArraySize       = 1;
}

static void FillViewDescForTexture2DArray(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    viewDesc.Texture2DArray.MipSlice        = attachmentDesc.mipLevel;
    viewDesc.Texture2DArray.FirstArraySlice = attachmentDesc.arrayLayer;
    viewDesc.Texture2DArray.ArraySize       = 1;
}

static void FillViewDescForTexture2DMS(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
}

static void FillViewDescForTexture2DArrayMS(const AttachmentDescriptor& attachmentDesc, D3D11_RENDER_TARGET_VIEW_DESC& viewDesc)
{
    viewDesc.ViewDimension                      = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
    viewDesc.Texture2DMSArray.FirstArraySlice   = attachmentDesc.arrayLayer;
    viewDesc.Texture2DMSArray.ArraySize         = 1;
}

void D3D11RenderTarget::AttachTexture(Texture& texture, const AttachmentDescriptor& attachmentDesc)
{
    /* Get D3D texture object and apply resolution for MIP-map level */
    auto& textureD3D = LLGL_CAST(D3D11Texture&, texture);
    ValidateMipResolution(texture, attachmentDesc.mipLevel);

    /* Initialize RTV descriptor with attachment procedure and create RTV */
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    InitMemory(rtvDesc);

    rtvDesc.Format = textureD3D.GetFormat();

    /*
    If this is a multi-sample render target, but the target texture is not a multi-sample texture,
    an intermediate multi-sample texture is required, which will be 'blitted' (or 'resolved') after the render target was rendered.
    */
    if (HasMultiSampling() && !IsMultiSampleTexture(texture.GetType()))
    {
        /* Get RTV descriptor for intermediate multi-sample texture */
        switch (texture.GetType())
        {
            case TextureType::Texture2D:
                FillViewDescForTexture2DMS(attachmentDesc, rtvDesc);
                break;
            case TextureType::TextureCube:
            case TextureType::Texture2DArray:
            case TextureType::TextureCubeArray:
                FillViewDescForTexture2DArrayMS(attachmentDesc, rtvDesc);
                break;
            default:
                throw std::invalid_argument("failed to attach D3D11 texture to multi-sample render-target");
                break;
        }

        /* Recreate texture resource with multi-sampling */
        D3D11_TEXTURE2D_DESC texDesc;
        textureD3D.GetNative().tex2D->GetDesc(&texDesc);
        {
            texDesc.Width               = (texDesc.Width << attachmentDesc.mipLevel);
            texDesc.Height              = (texDesc.Height << attachmentDesc.mipLevel);
            texDesc.MipLevels           = 1;
            texDesc.SampleDesc.Count    = multiSamples_;
            texDesc.MiscFlags           = 0;
        }
        ComPtr<ID3D11Texture2D> tex2DMS;
        auto hr = device_->CreateTexture2D(&texDesc, nullptr, tex2DMS.ReleaseAndGetAddressOf());
        DXThrowIfFailed(hr, "failed to create D3D11 multi-sampled 2D-texture for render-target");

        /* Store multi-sampled texture, and reference to texture target */
        multiSampledAttachments_.push_back(
            {
                tex2DMS,
                textureD3D.GetNative().tex2D.Get(),
                D3D11CalcSubresource(attachmentDesc.mipLevel, attachmentDesc.arrayLayer, texDesc.MipLevels),
                texDesc.Format
            }
        );

        /* Create RTV for multi-sampled (intermediate) texture */
        CreateAndAppendRTV(tex2DMS.Get(), rtvDesc);
    }
    else
    {
        /* Get RTV descriptor for this the target texture */
        switch (texture.GetType())
        {
            case TextureType::Texture1D:
                FillViewDescForTexture1D(attachmentDesc, rtvDesc);
                break;
            case TextureType::Texture2D:
                FillViewDescForTexture2D(attachmentDesc, rtvDesc);
                break;
            case TextureType::Texture3D:
                FillViewDescForTexture3D(attachmentDesc, rtvDesc);
                break;
            case TextureType::Texture1DArray:
                FillViewDescForTexture1DArray(attachmentDesc, rtvDesc);
                break;
            case TextureType::TextureCube:
            case TextureType::Texture2DArray:
            case TextureType::TextureCubeArray:
                FillViewDescForTexture2DArray(attachmentDesc, rtvDesc);
                break;
            case TextureType::Texture2DMS:
                FillViewDescForTexture2DMS(attachmentDesc, rtvDesc);
                break;
            case TextureType::Texture2DMSArray:
                FillViewDescForTexture2DArrayMS(attachmentDesc, rtvDesc);
                break;
        }

        /* Create RTV for target texture */
        CreateAndAppendRTV(textureD3D.GetNative().resource.Get(), rtvDesc);
    }
}

void D3D11RenderTarget::CreateDepthStencilAndDSV(DXGI_FORMAT format)
{
    depthStencilFormat_ = format;
    HRESULT hr = 0;

    /* Create depth stencil texture */
    D3D11_TEXTURE2D_DESC texDesc;
    {
        texDesc.Width               = GetResolution().width;
        texDesc.Height              = GetResolution().height;
        texDesc.MipLevels           = 1;
        texDesc.ArraySize           = 1;
        texDesc.Format              = depthStencilFormat_;
        texDesc.SampleDesc.Count    = std::max(1u, multiSamples_);
        texDesc.SampleDesc.Quality  = 0;
        texDesc.Usage               = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags           = D3D11_BIND_DEPTH_STENCIL;
        texDesc.CPUAccessFlags      = 0;
        texDesc.MiscFlags           = 0;
    }
    hr = device_->CreateTexture2D(&texDesc, nullptr, depthStencil_.ReleaseAndGetAddressOf());
    DXThrowIfFailed(hr, "failed to create D3D11 depth-texture for render-target");

    /* Create DSV */
    hr = device_->CreateDepthStencilView(depthStencil_.Get(), nullptr, depthStencilView_.ReleaseAndGetAddressOf());
    DXThrowIfFailed(hr, "failed to create D3D11 depth-stencil-view (DSV) for render-target");
}

void D3D11RenderTarget::CreateAndAppendRTV(ID3D11Resource* resource, const D3D11_RENDER_TARGET_VIEW_DESC& rtvDesc)
{
    ComPtr<ID3D11RenderTargetView> rtv;

    auto hr = device_->CreateRenderTargetView(resource, &rtvDesc, rtv.ReleaseAndGetAddressOf());
    DXThrowIfFailed(hr, "failed to create D3D11 render-target-view (RTV)");

    renderTargetViews_.push_back(rtv);
    renderTargetViewsRef_.push_back(rtv.Get());
}

bool D3D11RenderTarget::HasMultiSampling() const
{
    return (multiSamples_ > 1);
}


} // /namespace LLGL



// ================================================================================
