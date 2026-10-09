#pragma once

#include "EditorViewportClient.h"
#include "Render/ForwardViewTargets.h"

class FForwardEditorViewportClient : public FEditorViewportClient
{
public:
	virtual FTexture2D* GetColorTarget() const override { return ViewTargets.SceneColor.get(); }
	virtual FTexture2D* SwapSceneColorAndBind(TUniquePtr<FTexture2D>& Output) override { return ViewTargets.SwapSceneColorAndBind(Output); }
	virtual FTexture2D* GetDepthTarget() const override { return ViewTargets.Depth.get(); }
	FForwardViewTargets& GetViewTargets() { return ViewTargets; }
	const FForwardViewTargets& GetViewTargets() const { return ViewTargets; }

	virtual void Resize(uint32 InWidth, uint32 InHeight) override;

private:
	FForwardViewTargets ViewTargets;
};
