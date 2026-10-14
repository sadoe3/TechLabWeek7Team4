#pragma once

#include "Render/Renderer.h"
#include "Render/RenderingInfo.h"
#include "Render/PipelineState.h"

class FWorldNormalRenderer
{
public:
	void Init(FRenderer* InRenderer);
	bool OnRender(
		FTexture2D* ColorTarget, 
		FTexture2D* NormalTarget, 
		FTexture2D* DepthTarget
	);

private:
	FRenderer* Renderer = nullptr;
	FShaderProgram* Shader = nullptr;
	FPipelineState WorldNormalPipelineState;
};