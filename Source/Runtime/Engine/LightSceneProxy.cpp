#include "EnginePCH.h"
#include "Engine/LightSceneProxy.h"
#include "Component/PointLightComponent.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/SpotLightComponent.h"

#include <algorithm>
#include <cmath>

void FLightSceneProxy::UpdateSpotSphere(float Radius, float OuterAngle)
{
	if (Radius == CachedSpotRadius && OuterAngle == CachedSpotOuterAngle)
		return;

	CachedSpotRadius = Radius;
	CachedSpotOuterAngle = OuterAngle;
	const float CosOuter = std::cos(OuterAngle);
	if (OuterAngle <= PI / 4.0f)
	{
		SpotSphereOffset = Radius / (2.0f * CosOuter);
		CullingSphereRadius = SpotSphereOffset;
	}
	else
	{
		SpotSphereOffset = Radius * CosOuter;
		CullingSphereRadius = Radius * std::sin(OuterAngle);
	}
}

void FLightSceneProxy::Update()
{
	if (!Component)
	{
		bEnabled = false;
		return;
	}

	// 모든 광원에 공통으로 필요한 값을 먼저 갱신한다.
	LightType = Component->GetLightType();
	const FVector& Color = Component->GetLightColor();
	const float Intensity = std::max(0.0f, Component->GetIntensity());

	bEnabled = Component->IsEnabled() && Intensity > 0.0f;
	bVisible = Component->IsVisible();
	bHiddenInGame = Component->IsHiddenInGame();

	switch (LightType)
	{
	case ELightType::Ambient:
	{
		RenderData = FAmbientLightRenderData{ Color, Intensity };
		break;
	}
	case ELightType::Directional:
	{
		auto* Light = Cast<UDirectionalLightComponent>(Component);
		if (!Light)
		{
			bEnabled = false;
			return;
		}

		RenderData = FDirectionalLightRenderData{ Color, Intensity, Light->GetLightDirection() };
		break;
	}
	case ELightType::Point:
	{
		auto* Light = Cast<UPointLightComponent>(Component);

		if (!Light)
		{
			bEnabled = false;
			return;
		}

		const FVector Position = Component->GetWorldLocation();
		const float Radius = std::max(0.0f, Light->GetAttenuationRadius());
		RenderData = FPointLightRenderData{ Color, Intensity, Position, Radius };
		CullingSphereCenter = Position;
		CullingSphereRadius = Radius;
		bEnabled = bEnabled && Radius > 0.0f;
		break;
	}
	case ELightType::Spot:
	{
		auto* Light = Cast<USpotLightComponent>(Component);
		if (!Light)
		{
			bEnabled = false;
			return;
		}

		const FVector Position = Light->GetWorldLocation();
		const float Radius = std::max(0.0f, Light->GetAttenuationRadius());
		// 셰이딩과 컬링이 같은 원뿔을 사용한다. 각도 범위는 기존 디버그 표시와 같다.
		const float OuterDegrees = std::clamp(Light->GetOuterConeAngle(), 0.0f, 89.0f);
		const float OuterAngle = OuterDegrees * PI / 180.0f;
		const float InnerAngle = std::clamp(Light->GetInnerConeAngle(), 0.0f, OuterDegrees) * PI / 180.0f;
		const FVector Direction = Light->GetLightDirection();
		UpdateSpotSphere(Radius, OuterAngle);
		CullingSphereCenter = Position + Direction * SpotSphereOffset;
		RenderData = FSpotLightRenderData{ Color, Intensity, Position, Radius, Direction, InnerAngle, OuterAngle };
		bEnabled = bEnabled && Radius > 0.0f;
		break;
	}
	}
}
