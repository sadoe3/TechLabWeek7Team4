#include "EnginePCH.h"
#include "ForwardEditorViewportClient.h"

void FForwardEditorViewportClient::Resize(uint32 InWidth, uint32 InHeight)
{
    if (Width == InWidth && Height == InHeight &&
        ViewTargets.IsValidFor(InWidth, InHeight)) {
        return;
    }

    const uint32 OldWidth = Width;

    Width = InWidth;
    Height = InHeight;

    ViewTargets.Resize(Width, Height);

    if (Height > 0) {
        SetAspectRatio(static_cast<float>(Width) / static_cast<float>(Height));
    }

    if (!bIsPerspective && OldWidth > 1 && Width > 1) {
        // 화면 크기 변경 시 직교 배율 유지
        OrthoWidth *= static_cast<float>(Width) / static_cast<float>(OldWidth);
    }
}
