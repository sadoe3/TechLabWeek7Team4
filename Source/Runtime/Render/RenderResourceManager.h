#pragma once

#include "Shader.h"
#include "PipelineState.h"

// 렌더링에 사용할 파이프라인 종류 정의
enum class EPSOType : uint8
{
	StaticMesh_Opaque,
	StaticMesh_Translucent,  // 반투명
	StaticMesh_Wireframe,    
	Particle_AlphaBlend,     
	Particle_Additive,       
	Skybox,                  
	Grid,                    
	Outline_Mask,            
	Outline_Draw,
	StaticMesh_GBuffer,
	DeferredLighting,
	DeferredPointLighting,
	ToneMap,
	FireBall,
	Billboard,
	StaticMesh_Depth,
	Count
};

class FRenderResourceManager
{
public:
	static FRenderResourceManager& Get()
	{
		static FRenderResourceManager* Instance = new FRenderResourceManager();
		return *Instance;
	}

	static void Init()
	{
		Get().ScanShaders("Resources/Shader");
		Get().InitPipelineStates(); // PSO 초기화 추가
	}

	void ScanShaders(const fs::path& ShaderRoot);
	static void Shutdown();
	static FShaderProgram* GetShaderProgram(const FString& InPath, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());

	static FPipelineState* GetPSO(const EPSOType& InType, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());
private:
	
	void LoadOrCompileShader(const FString& Path, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());
	void InitPipelineStates();

	TUniquePtr<FPipelineState> CreatePSO(EPSOType InType, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());

	TMap<FString, TUniquePtr<FVertexShader>> VertexShaderMap;
	TMap<FString, TUniquePtr<FPixelShader>>  PixelShaderMap;
	TMap<FString, TUniquePtr<FComputeShader>> ComputeShaderMap;
	TMap<FString, TUniquePtr<FShaderProgram>> ShaderProgramMap;

	TMap<EPSOType, TUniquePtr<FPipelineState>> PipelineStateMap;
	TMap<FString, TUniquePtr<FPipelineState>> VariantPSOMap;
};
