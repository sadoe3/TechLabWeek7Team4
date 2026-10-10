#pragma once

#include "GameViewportClient.h"
#include "Engine/ViewportClient.h"
#include "Render/ForwardViewTargets.h"

class FForwardGameViewportClient : public FGameViewportClient
{
public:
	FForwardGameViewportClient() = default;
	virtual ~FForwardGameViewportClient() override = default;

	// 렌더 타깃 및 크기 관리
	using FGameViewportClient::Resize; // name hiding때문에 추가
	virtual void Resize(uint32 InWidth, uint32 InHeight) override;
	virtual FTexture2D* GetColorTarget() const override { return ViewTargets.SceneColor.get(); }
	virtual FTexture2D* SwapSceneColorAndBind(TUniquePtr<FTexture2D>& Output) override { return ViewTargets.SwapSceneColorAndBind(Output); }
	virtual FTexture2D* GetDepthTarget() const override { return ViewTargets.Depth.get(); }
	FForwardViewTargets& GetViewTargets() { return ViewTargets; }
	const FForwardViewTargets& GetViewTargets() const { return ViewTargets; }
	
protected:
	virtual void ResetViewTargets() override { ViewTargets.Reset(); }

private:
	FForwardViewTargets ViewTargets;
};
