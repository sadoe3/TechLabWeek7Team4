#include "EnginePCH.h"
#include "Engine/LightSceneProxy.h"
#include "Component/PointLightComponent.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/SpotLightComponent.h"

#include <algorithm>

void FLightSceneProxy::Update()
{
	if (!Component)
	{
		bEnabled = false;
		return;
	}

	// 모든 광원에 공통으로 필요한 값을 먼저 갱신한다.
	LightType = Component->GetLightType();
	const FVector4& Color = Component->GetLightColor();
	const float Intensity = std::max(0.0f, Component->GetIntensity());
	RenderData.ColorIntensity = FVector4(std::max(0.0f, Color.X), std::max(0.0f, Color.Y), std::max(0.0f, Color.Z), Intensity);

	bEnabled = Component->IsEnabled() && Intensity > 0.0f;
	bVisible = Component->IsVisible();
	bHiddenInGame = Component->IsHiddenInGame();

	switch (LightType)
	{
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
		RenderData.PositionRadius = FVector4(Position.X, Position.Y, Position.Z, Radius);
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
		RenderData.PositionRadius = FVector4(Position.X, Position.Y, Position.Z, Radius);
		Direction = Light->GetLightDirection();
		InnerConeAngle = Light->GetInnerConeAngle();
		OuterConeAngle = Light->GetOuterConeAngle();
		Falloff = Light->GetFalloff();
		bEnabled = bEnabled && Radius > 0.0f;
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

		Direction = Light->GetLightDirection();
		RenderData.PositionRadius = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		break;
	}
	case ELightType::Ambient:
	{
		// 공통 정보만 사용하므로 자식 타입으로 캐스팅할 필요가 없다.
		RenderData.PositionRadius = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		break;
	}
	}
}
