#include "EnginePCH.h"
#include "Component/SpotLightComponent.h"
#include "Render/LineBatcher.h"
#include "Engine/World.h"
#include "Engine/LightSceneProxy.h"

void USpotLightComponent::OnPropertyChanged(const FString& PropertyName)
{
	Super::OnPropertyChanged(PropertyName);

	if (PropertyName == "InnerConeAngle" || PropertyName == "OuterConeAngle")
	{
		OuterConeAngle = FMath::Clamp(OuterConeAngle, 1.0f, 80.0f);
		InnerConeAngle = FMath::Clamp(InnerConeAngle, 1.0f, OuterConeAngle);
	}
}

namespace
{
	constexpr int32 CONE_SEGMENT_COUNT = 24;
	constexpr int32 CONE_SIDE_LINE_COUNT = 24;
	constexpr int32 CAP_ARC_SEGMENT_COUNT = 16;

	// Outer/Inner 각도와 광원으로부터의 거리 반경으로 제한되는 범위를 그린다.
	void AddWireCone(FLineBatcher* LineBatcher, const FVector& Apex, const FVector& Forward, const FVector& Right, const FVector& Up, float HalfAngleDegrees, float Range, const FVector4& Color)
	{
		const float HalfAngle = FMath::DegreesToRadians(HalfAngleDegrees);
		const float RingRadius = std::sin(HalfAngle) * Range;
		const FVector RingCenter = Apex + Forward * (std::cos(HalfAngle) * Range);

		// 원뿔 옆면과 반경 구가 만나는 원.
		for (int32 Index = 0; Index < CONE_SEGMENT_COUNT; ++Index)
		{
			const float AngleA = 2.0f * PI * Index / CONE_SEGMENT_COUNT;
			const float AngleB = 2.0f * PI * (Index + 1) / CONE_SEGMENT_COUNT;
			const FVector A = RingCenter + (Right * std::cos(AngleA) + Up * std::sin(AngleA)) * RingRadius;
			const FVector B = RingCenter + (Right * std::cos(AngleB) + Up * std::sin(AngleB)) * RingRadius;
			LineBatcher->AddLine(A, B, Color);
		}

		// 옆선은 거리 반경에서 끝나며, 끝면은 평면이 아닌 구면이다.
		for (int32 Index = 0; Index < CONE_SIDE_LINE_COUNT; ++Index)
		{
			const float Angle = 2.0f * PI * Index / CONE_SIDE_LINE_COUNT;
			const FVector Radial = Right * std::cos(Angle) + Up * std::sin(Angle);
			LineBatcher->AddLine(Apex, RingCenter + Radial * RingRadius, Color);

			// 네 방향의 호로 둥근 끝면을 표시한다.
			if (Index % (CONE_SIDE_LINE_COUNT / 4) == 0)
			{
				FVector Previous = Apex + Forward * Range;
				for (int32 Segment = 1; Segment <= CAP_ARC_SEGMENT_COUNT; ++Segment)
				{
					const float ArcAngle = HalfAngle * Segment / CAP_ARC_SEGMENT_COUNT;
					const FVector Point = Apex + (Forward * std::cos(ArcAngle) + Radial * std::sin(ArcAngle)) * Range;
					LineBatcher->AddLine(Previous, Point, Color);
					Previous = Point;
				}
			}
		}
	}
}

void USpotLightComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
	if (!LineBatcher || !(AttenuationRadius > 0.0f))
	{
		return;
	}

	FTransform WorldTransform;
	WorldTransform.Location = GetWorldLocation();
	WorldTransform.Rotation = GetWorldRotation();

	FVector Apex = WorldTransform.Location;
	FVector Forward = WorldTransform.GetForward();
	FVector Right = WorldTransform.GetRight();
	FVector Up = WorldTransform.GetUp();

	// 에디터에서 값을 직접 드래그하므로 범위를 신뢰할 수 없다.
	// 조명 데이터와 동일한 각도 범위를 적용한다.
	const float SafeOuterAngle = FMath::Clamp(OuterConeAngle, 0.0f, 89.0f);
	const float SafeInnerAngle = FMath::Clamp(InnerConeAngle, 0.0f, SafeOuterAngle);
	const float SafeRadius = AttenuationRadius;

	// 바깥 원뿔은 라이트 색 그대로, 안쪽은 구분되도록 흐리게
	AddWireCone(LineBatcher, Apex, Forward, Right, Up, SafeOuterAngle, SafeRadius, LightColor);

	FVector4 InnerColor(LightColor.X * 0.5f, LightColor.Y * 0.5f, LightColor.Z * 0.5f, 1.0f);

	AddWireCone(LineBatcher, Apex, Forward, Right, Up, SafeInnerAngle, SafeRadius, InnerColor);

	// Spot Light의 근사 구를 보기 위해서 그린다. (기능적으로 하는 역할은 없어서 나중에 지워도 된다.)
	if (!GetWorld())
		return;
	for (const FLightSceneProxy* Proxy : GetWorld()->GetScene().GetLights())
	{
		if (Proxy->GetComponent() != this)
			continue;
		const auto* Data = std::get_if<FSpotLightRenderData>(&Proxy->GetRenderData());
		if (!Data)
			return;

		const FVector Center = Proxy->GetCullingSphereCenter();
		const float Radius = Proxy->GetCullingSphereRadius();
		const FVector4 SphereColor(0.0f, 1.0f, 1.0f, 1.0f);
		constexpr int32 SphereSegments = 64;
		for (int32 Index = 0; Index < SphereSegments; ++Index)
		{
			const float A = 2.0f * PI * Index / SphereSegments;
			const float B = 2.0f * PI * (Index + 1) / SphereSegments;
			const float CA = std::cos(A) * Radius;
			const float SA = std::sin(A) * Radius;
			const float CB = std::cos(B) * Radius;
			const float SB = std::sin(B) * Radius;
			LineBatcher->AddLine(Center + FVector(CA, SA, 0.0f), Center + FVector(CB, SB, 0.0f), SphereColor);
			LineBatcher->AddLine(Center + FVector(CA, 0.0f, SA), Center + FVector(CB, 0.0f, SB), SphereColor);
			LineBatcher->AddLine(Center + FVector(0.0f, CA, SA), Center + FVector(0.0f, CB, SB), SphereColor);
		}
		LineBatcher->AddLine(Center, Data->Location, FVector4(1.0f, 0.0f, 1.0f, 1.0f));

		return;
	}
}
