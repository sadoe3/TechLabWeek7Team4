#pragma once

#include "Math/Vector4.h"
#include "Component/LightComponent.h"

// class UPointLightComponent;
class ULightComponent;

struct FPointLightRenderData
{
	FVector4 PositionRadius;
	FVector4 ColorIntensity;
};

inline constexpr uint32 MaxDeferredPointLights = 32;

// 월드의 라이트 컴포넌트를 렌더러가 읽을 수 있는 값으로 복사한다.
class FLightSceneProxy
{
public:
	// explicit FLightSceneProxy(UPointLightComponent* InComponent) : Component(InComponent) { Update(); }
	explicit FLightSceneProxy(ULightComponent* InComponent) : Component(InComponent) { Update(); }
	void Update();
	// UPointLightComponent* GetComponent() const { return Component; }
	ULightComponent* GetComponent() const { return Component; }
	ELightType GetLightType() const { return LightType; }
	bool IsEnabled() const { return bEnabled; }
	bool IsShown(bool bGameView) const { return bVisible && !(bGameView && bHiddenInGame); }
	const FPointLightRenderData& GetRenderData() const { return RenderData; }
	const FVector& GetDirection() const { return Direction; }
	float GetInnerConeAngle() const { return InnerConeAngle; }
	float GetOuterConeAngle() const { return OuterConeAngle; }
	float GetFalloff() const { return Falloff; }

private:
	// UPointLightComponent* Component = nullptr;
	ULightComponent* Component = nullptr;
	ELightType LightType = ELightType::Point;
	FPointLightRenderData RenderData{};
	FVector Direction{};
	// Spot 원뿔의 반각은 컴포넌트와 동일하게 도 단위로 보관한다.
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 0.0f;
	float Falloff = 0.0f;
	bool bEnabled = false;
	bool bVisible = true;
	bool bHiddenInGame = false;
};
