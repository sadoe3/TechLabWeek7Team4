#pragma once

#include "Math/Transform.h"
#include "Render/Buffer.h"
#include "Render/PipelineState.h"

class ULightComponent;
class UStaticMesh;
class FLineBatcher;

class FLightVisualizer
{
public:
	void Init();
	void Draw(ULightComponent* Light, const FMatrix& ViewProj, FLineBatcher* LineBatcher);

private:
	void DrawDirectionArrow(const FVector& Location, const FRotator& Rotation, const FVector& Color, const FMatrix& ViewProj);

	UStaticMesh* ArrowMesh = nullptr;
	FShaderProgram* Shader = nullptr;
	FPipelineState PipelineState;
	TUniquePtr<FConstantBuffer> CB;
};
