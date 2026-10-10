#include "EnginePCH.h"
#include "LightGridDebugRenderer.h"

#include "Render/RenderCommand.h"
#include "Render/RenderResourceManager.h"
#include "Text/Font.h"
#include "Render/LightCulling/TileLightCullingResources.h"
#include "Render/Texture2D.h"

bool FLightGridDebugRenderer::Init(UFont* InFont)
{
    if (!InFont || !InFont->AtlasTexture || InFont->Width <= 0.0f || InFont->Height <= 0.0f)
    {
        return false;
    }

    FLightGridDebugFontConstants FontData{};
    FontData.AtlasSize[0] = InFont->Width;
    FontData.AtlasSize[1] = InFont->Height;
    FontData.DistanceRange = InFont->DistanceRange;

    // 기존 폰트에서 숫자 0~9의 atlas 영역만 가져온다.
    for (uint32 Digit = 0; Digit < 10; ++Digit)
    {
        const uint32 Unicode = static_cast<uint32>('0') + Digit;
        const FGlyphInfo* Glyph = InFont->GlyphMap.Find(Unicode);

        if (!Glyph || !Glyph->bHasBounds)
        {
            return false;
        }

        if (Glyph->AtlasRight <= Glyph->AtlasLeft || Glyph->AtlasTop <= Glyph->AtlasBottom)
        {
            return false;
        }

        // TextRenderer와 동일하게 atlas의 Y 좌표를 UV 좌표로 변환한다.
        FontData.DigitUV[Digit][0] = Glyph->AtlasLeft / InFont->Width;
        FontData.DigitUV[Digit][1] = 1.0f - Glyph->AtlasTop / InFont->Height;
        FontData.DigitUV[Digit][2] = Glyph->AtlasRight / InFont->Width;
        FontData.DigitUV[Digit][3] = 1.0f - Glyph->AtlasBottom / InFont->Height;
    }

    Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/LightGridDebugShader.hlsl");

    if (!Shader || !Shader->VertexShader || !Shader->PixelShader)
    {
        return false;
    }

    ViewConstantsBuffer = RenderCommand::CreateConstantBuffer(sizeof(FLightGridDebugConstants));
    FontConstantsBuffer = RenderCommand::CreateConstantBuffer(sizeof(FLightGridDebugFontConstants));

    if (!ViewConstantsBuffer || !ViewConstantsBuffer->GetBuffer() || !FontConstantsBuffer || !FontConstantsBuffer->GetBuffer())
    {
        return false;
    }

    // 숫자 UV와 atlas 정보는 초기화할 때 한 번만 업로드한다.
    RenderCommand::UpdateBufferData(FontConstantsBuffer.get(), &FontData, sizeof(FontData));

    PipelineState.Shader = Shader;
    PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    PipelineState.RasterizerState = ERasterizerState::SolidNone;
    PipelineState.BlendState = EBlendState::Opaque;
    PipelineState.DepthStencilState = EDepthStencilState::Disabled;

    Font = InFont;
    return true;
}

bool FLightGridDebugRenderer::OnRender(const FTileLightCullingResources& Resources, FTexture2D* ColorTarget, FTexture2D* DepthTarget)
{
    if (!bEnabled || !ColorTarget || !Resources.IsValid())
    {
        return false;
    }

    if (!Shader || !Font || !ViewConstantsBuffer || !FontConstantsBuffer)
    {
        return false;
    }

    const uint32 Width = ColorTarget->GetWidth();
    const uint32 Height = ColorTarget->GetHeight();

    if (Width == 0 || Height == 0)
    {
        return false;
    }

    FLightGridDebugConstants Constants{};
    Constants.ViewSize[0] = Width;
    Constants.ViewSize[1] = Height;
    Constants.TileCountX = Resources.TileCountX;
    Constants.MaxLightsPerTile = Resources.MaxLightsPerTile;

    // 현재 컬링과 동일한 크기의 타일 배치인지 확인한다.
    const uint32 ExpectedTileCountX = Width / Constants.TileSize + (Width % Constants.TileSize != 0);
    const uint32 ExpectedTileCountY = Height / Constants.TileSize + (Height % Constants.TileSize != 0);

    if (Resources.TileCountX != ExpectedTileCountX || Resources.TileCountY != ExpectedTileCountY)
    {
        return false;
    }

    RenderCommand::UpdateBufferData(ViewConstantsBuffer.get(), &Constants, sizeof(Constants));

    RenderCommand::BindPipelineState(PipelineState);
    RenderCommand::SetRenderTargets(ColorTarget, nullptr);
    RenderCommand::SetViewport(0, 0, Width, Height);

    // 정점 위치는 셰이더에서 SV_VertexID로 생성한다.
    RenderCommand::BindVertexBuffer(nullptr);

    RenderCommand::BindConstantBuffer(0, ViewConstantsBuffer.get(), EShaderBindFlagBits::Pixel);
    RenderCommand::BindConstantBuffer(1, FontConstantsBuffer.get(), EShaderBindFlagBits::Pixel);

    RenderCommand::BindShaderResource(0, Resources.TileLightCountSRV.Get(), EShaderBindFlagBits::Pixel);
    RenderCommand::BindShaderResource(1, Font->AtlasTexture, EShaderBindFlagBits::Pixel);
    RenderCommand::BindSamplerState(0, ESamplerState::LinearClamp, EShaderBindFlagBits::Pixel);

    // 삼각형 두 개로 전체 화면 quad를 그린다.
    RenderCommand::Draw(6);

    // 다음 컬링에서 같은 버퍼를 UAV로 사용할 수 있도록 SRV 연결을 해제한다.
    RenderCommand::UnbindShaderResources(0, 2, EShaderBindFlagBits::Pixel);
    RenderCommand::BindConstantBuffer(0, nullptr, EShaderBindFlagBits::Pixel);
    RenderCommand::BindConstantBuffer(1, nullptr, EShaderBindFlagBits::Pixel);

    // 기존 SceneDepthRenderer처럼 후속 렌더링의 타깃 연결을 복구한다.
    RenderCommand::SetRenderTargets(ColorTarget, DepthTarget);
    return true;
}