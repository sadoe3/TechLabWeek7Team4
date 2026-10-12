#include "EnginePCH.h"
#include "Component/PointLightComponent.h"

#include "Render/LineBatcher.h"
#include <cmath>

void UPointLightComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
	if (!LineBatcher || !(AttenuationRadius > 0.0f))
		return;

	const FVector Center = GetWorldLocation();
	constexpr int32 SegmentCount = 32;

	// XY, XZ, YZ 평면의 원으로 구형 범위를 표시한다.
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const float AngleA = 2.0f * PI * Index / SegmentCount;
		const float AngleB = 2.0f * PI * (Index + 1) / SegmentCount;
		const float CosA = std::cos(AngleA) * AttenuationRadius;
		const float SinA = std::sin(AngleA) * AttenuationRadius;
		const float CosB = std::cos(AngleB) * AttenuationRadius;
		const float SinB = std::sin(AngleB) * AttenuationRadius;

		LineBatcher->AddLine(Center + FVector(CosA, SinA, 0.0f),
			Center + FVector(CosB, SinB, 0.0f), LightColor);
		LineBatcher->AddLine(Center + FVector(CosA, 0.0f, SinA),
			Center + FVector(CosB, 0.0f, SinB), LightColor);
		LineBatcher->AddLine(Center + FVector(0.0f, CosA, SinA),
			Center + FVector(0.0f, CosB, SinB), LightColor);
	}
}
