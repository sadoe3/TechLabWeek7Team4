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
		RenderData = FSpotLightRenderData{ Color, Intensity, Position, Radius, Light->GetLightDirection(), Light->GetInnerConeAngle() * PI / 180.0f, Light->GetOuterConeAngle() * PI / 180.0f, Light->GetFalloff() };
		bEnabled = bEnabled && Radius > 0.0f;
		break;
	}
	}
}
