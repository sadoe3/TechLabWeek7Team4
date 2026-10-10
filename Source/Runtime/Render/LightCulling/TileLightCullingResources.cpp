#include "EnginePCH.h"
#include "TileLightCullingResources.h"
#include "TileLightCulling.h"
#include "Render/RenderCommand.h"

void FTileLightCullingResources::Reset()
{
    LightSpheresSRV.Reset();

    TileLightCountSRV.Reset();
    TileLightCountUAV.Reset();

    TileLightIndicesSRV.Reset();
    TileLightIndicesUAV.Reset();

    LightSpheres.reset();
    TileLightCount.reset();
    TileLightIndices.reset();

    LightCapacity = 0;
    TileCountX = 0;
    TileCountY = 0;
    MaxLightsPerTile = 0;
}

bool FTileLightCullingResources::IsValid() const
{
    const bool bBuffersValid =
                                LightSpheres && LightSpheres->GetBuffer() &&
                                TileLightCount && TileLightCount->GetBuffer() &&
                                TileLightIndices && TileLightIndices->GetBuffer();

    const bool bViewsValid =
                                LightSpheresSRV.Get() != nullptr &&
                                TileLightCountSRV.Get() != nullptr &&
                                TileLightCountUAV.Get() != nullptr &&
                                TileLightIndicesSRV.Get() != nullptr &&
                                TileLightIndicesUAV.Get() != nullptr;

    const bool bDimensionsValid =
                                LightCapacity > 0 &&
                                TileCountX > 0 &&
                                TileCountY > 0 &&
                                MaxLightsPerTile > 0;

    return bBuffersValid && bViewsValid && bDimensionsValid;
}


/*
크기를 검증
기존 자원이 유효하고 용량이 충분하면 재사용
재생성이 필요하면 기존 자원을 정리하고 버퍼·뷰를 생성
하나라도 실패하면 전체를 정리하고 false를 반환
*/
bool FTileLightCullingResources::EnsureCapacity(uint32 InTileCountX, uint32 InTileCountY, uint32 RequiredLightCount, uint32 InMaxLightsPerTile)
{
    if (InTileCountX == 0 || InTileCountY == 0 || InMaxLightsPerTile == 0)
    {
        Reset();
        return false;
    }

    // 실제 라이트가 없어도 바인딩할 입력 버퍼는 최소 1칸 확보합니다.
    const uint32 RequiredCapacity = RequiredLightCount > 0 ? RequiredLightCount : 1;
    constexpr uint64 MaxBufferBytes = (std::numeric_limits<uint32>::max)();
    const uint64 MaxUintElements = MaxBufferBytes / sizeof(uint32);
    const uint64 NumTiles = static_cast<uint64>(InTileCountX) * InTileCountY;

    if (NumTiles > MaxUintElements || RequiredCapacity > MaxBufferBytes / sizeof(FCullingLight))
    {
        HTR_LOG(Error, "[TileLightResources] buffer size exceeds uint32 range");
        Reset();
        return false;
    }

    const uint64 NumIndices = NumTiles * InMaxLightsPerTile;

    if (NumIndices > MaxUintElements)
    {
        HTR_LOG(Error, "[TileLightResources] index buffer size exceeds uint32 range");
        Reset();
        return false;
    }

    if (IsValid() && TileCountX == InTileCountX && TileCountY == InTileCountY && MaxLightsPerTile == InMaxLightsPerTile && LightCapacity >= RequiredCapacity)
        return true;

    Reset();

    ID3D11Device* Device = RenderCommand::GetDevice();

    // 이 자원 묶음 내부에서만 사용하는 버퍼·뷰 생성 절차
    const auto CreateResource = [Device](uint32 ElementCount, uint32 ElementStride, D3D11_USAGE Usage, TUniquePtr<FStructuredBuffer>& OutBuffer, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& OutSRV, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* OutUAV) -> bool
        {
            OutBuffer = RenderCommand::CreateStructuredBuffer(ElementCount, ElementStride, Usage, OutUAV != nullptr);

            if (!OutBuffer || !OutBuffer->GetBuffer())
                return false;

            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
            SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            SRVDesc.Buffer.NumElements = ElementCount;

            if (FAILED(Device->CreateShaderResourceView(OutBuffer->GetBuffer(), &SRVDesc, OutSRV.GetAddressOf())))
                return false;

            if (OutUAV)
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
                UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
                UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                UAVDesc.Buffer.NumElements = ElementCount;

                if (FAILED(Device->CreateUnorderedAccessView(OutBuffer->GetBuffer(), &UAVDesc, OutUAV->GetAddressOf())))
                    return false;
            }

            return true;
        };

    const bool bCreated =
        CreateResource(RequiredCapacity, sizeof(FCullingLight), D3D11_USAGE_DYNAMIC, LightSpheres, LightSpheresSRV, nullptr) &&
        CreateResource(static_cast<uint32>(NumTiles), sizeof(uint32), D3D11_USAGE_DEFAULT, TileLightCount, TileLightCountSRV, &TileLightCountUAV) &&
        CreateResource(static_cast<uint32>(NumIndices), sizeof(uint32), D3D11_USAGE_DEFAULT, TileLightIndices, TileLightIndicesSRV, &TileLightIndicesUAV);

    if (!bCreated)
    {
        HTR_LOG(Error, "[TileLightResources] buffer or view creation failed");
        Reset();
        return false;
    }

    LightCapacity = RequiredCapacity;
    TileCountX = InTileCountX;
    TileCountY = InTileCountY;
    MaxLightsPerTile = InMaxLightsPerTile;

    return true;
}