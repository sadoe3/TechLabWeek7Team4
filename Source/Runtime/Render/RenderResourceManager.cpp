#include "EnginePCH.h"
#include "RenderResourceManager.h"

#include "RenderCommand.h"

#include "RenderUtil.h"



struct FPipelineTableEntry
{
	EPSOType Type;
	const char* ShaderPath;
	D3D11_PRIMITIVE_TOPOLOGY Topology;
	ERasterizerState RasterizerState;
	EBlendState BlendState;
	EDepthStencilState DepthStencilState;
};

constexpr FPipelineTableEntry PipelineTable[] =
{
	{ EPSOType::StaticMesh_Opaque,    "Resources/Shader/StaticMeshUberLit.hlsl",            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidBack, EBlendState::Opaque,       EDepthStencilState::Default },
	{ EPSOType::StaticMesh_Translucent,  "Resources/Shader/StaticMeshTranslucentShader.hlsl", D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidBack, EBlendState::AlphaBlend,   EDepthStencilState::ReadOnly },
	{ EPSOType::StaticMesh_Wireframe,    "Resources/Shader/StaticMeshUberLit.hlsl",    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::Wireframe, EBlendState::Opaque,       EDepthStencilState::Default },
	{ EPSOType::Particle_AlphaBlend,     "Resources/Shader/ParticleSubUVShader.hlsl", D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::AlphaBlend,   EDepthStencilState::ReadOnly },
	{ EPSOType::Particle_Additive,       "Resources/Shader/ParticleSubUVShader.hlsl", D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Additive,     EDepthStencilState::ReadOnly },
	{ EPSOType::Skybox,                  "Resources/Shader/SkyboxShader.hlsl",        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Opaque,       EDepthStencilState::ReadOnly },
	{ EPSOType::Grid,                    "Resources/Shader/GridShader.hlsl",          D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidBack, EBlendState::AlphaBlend,   EDepthStencilState::Default },
	{ EPSOType::Outline_Mask,            "Resources/Shader/OutlineShader.hlsl",       D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::NoColorWrite, EDepthStencilState::StencilMask },
	{ EPSOType::Outline_Draw,            "Resources/Shader/OutlineShader.hlsl",       D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Opaque,       EDepthStencilState::StencilOutline },
	{ EPSOType::StaticMesh_GBuffer,		 "Resources/Shader/StaticMeshGBufferShader.hlsl",	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,	ERasterizerState::SolidBack,	EBlendState::Opaque,  EDepthStencilState::Default },
	{ EPSOType::DeferredLighting,       "Resources/Shader/DeferredLightingShader.hlsl",    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Opaque,       EDepthStencilState::Disabled },
	{ EPSOType::DeferredPointLighting,  "Resources/Shader/DeferredLightingShader.hlsl",    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Additive,     EDepthStencilState::Disabled },
	{ EPSOType::ToneMap,                "Resources/Shader/ToneMapShader.hlsl",              D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Opaque,       EDepthStencilState::Disabled },
	{ EPSOType::FireBall,               "Resources/Shader/FireBallShader.hlsl",             D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::Additive,     EDepthStencilState::ReadOnly },
	{ EPSOType::Billboard,              "Resources/Shader/BillboardShader.hlsl",            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ERasterizerState::SolidNone, EBlendState::AlphaBlend,   EDepthStencilState::ReadOnly },
};

static_assert(sizeof(PipelineTable) / sizeof(PipelineTable[0]) == static_cast<size_t>(EPSOType::Count));

void FRenderResourceManager::InitPipelineStates()
{
	for (size_t i = 0; i < static_cast<size_t>(EPSOType::Count); ++i)
	{
		const EPSOType Type = static_cast<EPSOType>(i);
		PipelineStateMap[Type] = CreatePSO(Type, {});
	}
}

TUniquePtr<FPipelineState> FRenderResourceManager::CreatePSO(EPSOType InType, const TArray<FShaderMacro>& Macros)
{
	const auto& Entry = PipelineTable[static_cast<size_t>(InType)];

	auto State = MakeUnique<FPipelineState>();
	State->Shader = GetShaderProgram(Entry.ShaderPath, Macros);
	State->Topology = Entry.Topology;
	State->RasterizerState = Entry.RasterizerState;
	State->BlendState = Entry.BlendState;
	State->DepthStencilState = Entry.DepthStencilState;

	return State;
}

FPipelineState* FRenderResourceManager::GetPSO(const EPSOType& InType, const TArray<FShaderMacro>& Macros)
{
	if (Macros.IsEmpty())
	{
		if (auto* Found = Get().PipelineStateMap.Find(InType))
		{
			return Found->get();
		}
	}
	else
	{
		FString Key = std::to_string(static_cast<uint8>(InType)) + "|" + RenderUtil::MakeMacroKey(Macros);
		if (TUniquePtr<FPipelineState>* Found = Get().VariantPSOMap.Find(Key))
		{
			return Found->get();
		}
		else
		{
			TUniquePtr<FPipelineState> NewPSO = Get().CreatePSO(InType, Macros);
			FPipelineState* NewPSORaw = NewPSO.get();
			Get().VariantPSOMap[Key] = std::move(NewPSO);
			return NewPSORaw;
		}
	}
	return nullptr;
}

void FRenderResourceManager::ScanShaders(const fs::path& ShaderRoot)
{
	std::error_code ErrorCode;

	if (!fs::exists(ShaderRoot, ErrorCode))
	{
		HTR_LOG(Error, "[Shader] Scan Root Not Found : {}", ShaderRoot.generic_string());
		return;
	}

	for (const fs::directory_entry& Entry : fs::recursive_directory_iterator(ShaderRoot))
	{
		if (!Entry.is_regular_file()) continue;
		if (Entry.path().extension() != ".hlsl") continue;

		FString Path = Entry.path().generic_string();
		LoadOrCompileShader(Path);
	}
}

void FRenderResourceManager::Shutdown()
{
	Get().VertexShaderMap.Empty();
	Get().PixelShaderMap.Empty();
	Get().ShaderProgramMap.Empty();
}

FShaderProgram* FRenderResourceManager::GetShaderProgram(const FString& InPath, const TArray<FShaderMacro>& Macros)
{
	FString ShaderKey = InPath + RenderUtil::MakeMacroKey(Macros);

	if (TUniquePtr<FShaderProgram>* Found = Get().ShaderProgramMap.Find(ShaderKey))
		return Found->get();
	Get().LoadOrCompileShader(InPath, Macros);
	if (TUniquePtr<FShaderProgram>* Found = Get().ShaderProgramMap.Find(ShaderKey))
		return Found->get();

	HTR_LOG(Error, "[Shader] not found: {}", InPath);

	if (TUniquePtr<FShaderProgram>* Fallback =
		Get().ShaderProgramMap.Find("Resources/Shader/DefaultShader.hlsl"))
		return Fallback->get();

	return nullptr;
}




void FRenderResourceManager::LoadOrCompileShader(const FString& Path, const TArray<FShaderMacro>& Macros)
{ 
	FString VSCSOPath;
	FString PSCSOPath;
	FShaderByteCode VSCode = RenderUtil::GetOrCompile(Path, "mainVS", EShaderType::Vertex, VSCSOPath, Macros);
	FShaderByteCode PSCode = RenderUtil::GetOrCompile(Path, "mainPS", EShaderType::Pixel, PSCSOPath, Macros);

	if (!VSCode.IsValid() || !PSCode.IsValid())
	{
		HTR_LOG(Error, "[Shader] compile failed: {}", Path);
		return;
	}

	TUniquePtr<FVertexShader> Vs = RenderCommand::CreateVertexShader(VSCode);
	TUniquePtr<FPixelShader>  Ps = RenderCommand::CreatePixelShader(PSCode);

	if (!Vs || !Vs->IsValid() || !Ps || !Ps->IsValid())
	{
		HTR_LOG(Error, "[Shader] device create failed: {}", Path);
		return;
	}

	FVertexShader* VsRaw = Vs.get();
	FPixelShader* PsRaw = Ps.get();

	VertexShaderMap[VSCSOPath] = std::move(Vs);
	PixelShaderMap[PSCSOPath] = std::move(Ps);
	ShaderProgramMap[Path + RenderUtil::MakeMacroKey(Macros)] = MakeUnique<FShaderProgram>(VsRaw, PsRaw);

	HTR_LOG(Info, "[Shader] loaded: {}", Path);

}
