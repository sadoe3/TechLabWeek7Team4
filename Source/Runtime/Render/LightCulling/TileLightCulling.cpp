#include "EnginePCH.h"
#include "TileLightCulling.h"
#include "Render/RenderCommand.h"
#include "Render/RenderResourceManager.h"
#include "TileLightCullingResources.h"

bool FTileLightCulling::Init()
{
    ComputeShader = nullptr;
    ConstantsBuffer.reset();

    FShaderProgram* LoadedShader = FRenderResourceManager::GetShaderProgram("Resources/Shader/CS_TileLightCulling.hlsl");

    if (!LoadedShader || !LoadedShader->ComputeShader || !LoadedShader->ComputeShader->IsValid())
    {
        HTR_LOG(Error, "[TileLightCulling] compute shader is unavailable");
        return false;
    }

    ConstantsBuffer = RenderCommand::CreateConstantBuffer(sizeof(FTileCullingConstants));

    if (!ConstantsBuffer || !ConstantsBuffer->GetBuffer())
    {
        HTR_LOG(Error, "[TileLightCulling] constant buffer creation failed");
        ConstantsBuffer.reset();
        return false;
    }

    ComputeShader = LoadedShader;
    return true;
}

void FTileLightCulling::Execute(FTileLightCullingResources & Resources, const uint32 ViewWidth, const uint32 ViewHeight, const FMatrix & Projection, const FCullingLight * Lights, const uint32 LightCount)
{
    if (ViewWidth == 0 || ViewHeight == 0)
        return;

    // CS_TileLightCulling.hlsl의 TILE_SIZE와 일치해야 한다
    constexpr uint32 TileSize = 16;

    const uint32 TileCountX = ViewWidth / TileSize + (ViewWidth % TileSize != 0);
    const uint32 TileCountY = ViewHeight / TileSize + (ViewHeight % TileSize != 0);

    if (!Resources.EnsureCapacity(TileCountX, TileCountY, LightCount, MaxLightsPerTile))
        return;

    FTileCullingConstants Constants{};
    Constants.ViewSize[0] = ViewWidth;
    Constants.ViewSize[1] = ViewHeight;
    Constants.TileCountX = TileCountX;
    Constants.LightCount = LightCount;
    Constants.Projection = Projection;
    Constants.MaxLightsPerTile = MaxLightsPerTile;

    UploadInputs(Resources, Lights, Constants);

    // CS_TileLightCulling.hlsl에 선언된 레지스터 번호
    constexpr uint32 ConstantsSlot = 0;    // b0
    constexpr uint32 LightSpheresSlot = 0; // t0
    constexpr uint32 LightCountSlot = 0;   // u0
    constexpr uint32 LightIndicesSlot = 1; // u1

    RenderCommand::BindShaderProgram(ComputeShader);
    RenderCommand::BindConstantBuffer(ConstantsSlot, ConstantsBuffer.get(), EShaderBindFlagBits::Compute);
    RenderCommand::BindShaderResource(LightSpheresSlot, Resources.LightSpheresSRV.Get(), EShaderBindFlagBits::Compute);

    RenderCommand::BindComputeUAV(LightCountSlot, Resources.TileLightCountUAV.Get());
    RenderCommand::BindComputeUAV(LightIndicesSlot, Resources.TileLightIndicesUAV.Get());

    // 타일 하나당 스레드 그룹 하나를 실행
    // 타일 배치는 2차원이므로 Z는 1
    RenderCommand::Dispatch(TileCountX, TileCountY, 1);

    // 후속 렌더링에서 결과를 SRV로 읽을 수 있도록 UAV 연결을 해제
    RenderCommand::BindComputeUAV(LightCountSlot, nullptr);
    RenderCommand::BindComputeUAV(LightIndicesSlot, nullptr);

    // 이번 실행에서 사용한 입력 슬롯도 해제
    RenderCommand::UnbindShaderResources(LightSpheresSlot, 1, EShaderBindFlagBits::Compute);
    RenderCommand::BindConstantBuffer(ConstantsSlot, nullptr, EShaderBindFlagBits::Compute);
}


void FTileLightCulling::UploadInputs(FTileLightCullingResources& Resources, const FCullingLight* Lights, const FTileCullingConstants& Constants)
{
    // 라이트가 없으면 입력 배열 복사는 생략
    if (Constants.LightCount > 0)
    {
        const uint32 DataSize = Constants.LightCount * static_cast<uint32>(sizeof(FCullingLight));
        RenderCommand::UpdateBufferData(Resources.LightSpheres.get(), Lights, DataSize);
    }

    // 라이트가 없어도 LightCount = 0을 포함한 현재 뷰의 상수는 갱신
    RenderCommand::UpdateBufferData(ConstantsBuffer.get(), &Constants);
}

// 실제로 잘 동작하는지 확인하기 위해 사용되는 함수
void FTileLightCulling::DebugReadbackCounts(const FTileLightCullingResources& Resources)
{
    if (!Resources.IsValid())
        return;

    D3D11_BUFFER_DESC Desc{};
    Desc.ByteWidth = Resources.TileLightCount->GetBufferSize();
    Desc.Usage = D3D11_USAGE_STAGING;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    Microsoft::WRL::ComPtr<ID3D11Buffer> StagingBuffer;
    if (FAILED(RenderCommand::GetDevice()->CreateBuffer(&Desc, nullptr, StagingBuffer.GetAddressOf())))
    {
        HTR_LOG(Error, "[TileLightCulling] readback buffer creation failed");
        return;
    }

    ID3D11DeviceContext* Context = RenderCommand::GetContext();
    Context->CopyResource(StagingBuffer.Get(), Resources.TileLightCount->GetBuffer());

    // 시험용 동기 읽기입니다. 복사가 끝날 때까지 CPU가 기다릴 수 있습니다.
    D3D11_MAPPED_SUBRESOURCE Mapped{};
    if (FAILED(Context->Map(StagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &Mapped)))
    {
        HTR_LOG(Error, "[TileLightCulling] readback map failed");
        return;
    }

    const uint32* Counts = static_cast<const uint32*>(Mapped.pData);
    const uint32 TileCount = Resources.TileCountX * Resources.TileCountY;

    uint32 NonEmptyTiles = 0;
    uint32 MaxRawCount = 0;
    uint32 OverflowTiles = 0;

    for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
    {
        const uint32 RawCount = Counts[TileIndex];

        if (RawCount > 0)
            ++NonEmptyTiles;

        if (RawCount > MaxRawCount)
            MaxRawCount = RawCount;

        if (RawCount > Resources.MaxLightsPerTile)
            ++OverflowTiles;
    }

    Context->Unmap(StagingBuffer.Get(), 0);

    HTR_LOG(Info, "[TileLightCulling] tiles={}, nonEmpty={}, maxRawCount={}, overflowTiles={}", TileCount, NonEmptyTiles, MaxRawCount, OverflowTiles);
}