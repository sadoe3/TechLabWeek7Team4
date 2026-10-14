#include "EnginePCH.h"
#include "Editor/Rendering/WorldNormalRenderer.h"
#include "Render/RenderCommand.h"
#include "Render/RenderResourceManager.h"

void FWorldNormalRenderer::Init(FRenderer* InRenderer)
{
	Renderer = InRenderer;
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/WorldNormalShader.hlsl");

	WorldNormalPipelineState.Shader = Shader;
	WorldNormalPipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	WorldNormalPipelineState.RasterizerState = ERasterizerState::SolidNone;
	WorldNormalPipelineState.BlendState = EBlendState::Opaque;
	WorldNormalPipelineState.DepthStencilState = EDepthStencilState::Disabled;
}

bool FWorldNormalRenderer::OnRender(
	FTexture2D* ColorTarget,
	FTexture2D* NormalTarget,
	FTexture2D* DepthTarget
)
{
	RenderCommand::BindPipelineState(WorldNormalPipelineState);
	RenderCommand::SetRenderTargets(ColorTarget, nullptr);
	RenderCommand::BindShaderResource(0, NormalTarget, EShaderBindFlagBits::Pixel);
	RenderCommand::Draw(6);
	RenderCommand::BindShaderResource(0, static_cast<FTexture2D*>(nullptr), EShaderBindFlagBits::Pixel);
	RenderCommand::SetRenderTargets(ColorTarget, DepthTarget);
	return true;
}