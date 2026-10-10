#pragma once

#include "Core/Types.h"
#include "Render/Buffer.h"
#include "Render/PipelineState.h"

class UFont;
class FTexture2D;
class FShaderProgram;
struct FTileLightCullingResources;

// 뷰마다 갱신하는 히트맵 표시 정보
struct alignas(16) FLightGridDebugConstants
{
    uint32 ViewSize[2] = {};
    uint32 TileCountX = 0;
    uint32 TileSize = 16;

    uint32 MaxLightsPerTile = 0;
    float HeatMapMaxCount = 8.0f;
    float DigitHeight = 11.0f;
    float Padding = 0.0f;
};

// 기존 폰트에서 가져와 초기화할 때 한 번 업로드하는 정보
struct alignas(16) FLightGridDebugFontConstants
{
    // 숫자 0~9의 UV 사각형: left, top, right, bottom
    float DigitUV[10][4] = {};

    float AtlasSize[2] = {};
    float DistanceRange = 0.0f;
    float Padding = 0.0f;
};


// 콘솔에 LightGridDebug 1 을 통해서 활성화 할 수 있는 렌더러
// 이 렌더러는 각 타일별로 가지고있는 light의 수를 보여주고, 이를 색상으로도 보여준다
class FLightGridDebugRenderer
{
public:
    bool Init(UFont* InFont);
    bool OnRender(const FTileLightCullingResources& Resources, FTexture2D* ColorTarget, FTexture2D* DepthTarget);

    void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }
    bool IsEnabled() const { return bEnabled; }

private:
    bool bEnabled = false;

    // 기존 에셋과 셰이더 관리자가 소유합니다.
    UFont* Font = nullptr;
    FShaderProgram* Shader = nullptr;

    FPipelineState PipelineState;

    TUniquePtr<FConstantBuffer> ViewConstantsBuffer;
    TUniquePtr<FConstantBuffer> FontConstantsBuffer;
};

static_assert(sizeof(FLightGridDebugConstants) == 32);
static_assert(sizeof(FLightGridDebugFontConstants) == 176);