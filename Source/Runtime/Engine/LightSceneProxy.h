#pragma once

#include "Component/LightComponent.h"
#include <variant>

class ULightComponent;

struct FAmbientLightRenderData
{
	FVector Color;
	float Intensity;
};

struct FDirectionalLightRenderData
{
	FVector Color;
	float Intensity;
	FVector Direction;
	float Padding;
};

struct FPointLightRenderData
{
	FVector Color;
	float Intensity;
	FVector Location;
	float AttenuationRadius;
};

struct FSpotLightRenderData
{
	FVector Color;
	float Intensity;
	FVector Location;
	float AttenuationRadius;
	FVector Direction;
	float InnerConeAngle;
	float OuterConeAngle;
	float Falloff;
	float Padding[2];
};

inline constexpr uint32 MaxDirectionalLights = 8;
inline constexpr uint32 MaxPointLights = 8;
inline constexpr uint32 MaxSpotLights = 8;

// 월드의 라이트 컴포넌트를 렌더러가 읽을 수 있는 값으로 복사한다.
class FLightSceneProxy
{
	using FLightRenderData = std::variant<
		FAmbientLightRenderData,
		FDirectionalLightRenderData,
		FPointLightRenderData,
		FSpotLightRenderData>;

public:
	// explicit FLightSceneProxy(UPointLightComponent* InComponent) : Component(InComponent) { Update(); }
	explicit FLightSceneProxy(ULightComponent* InComponent) : Component(InComponent) { Update(); }
	void Update();
	// UPointLightComponent* GetComponent() const { return Component; }
	ULightComponent* GetComponent() const { return Component; }
	ELightType GetLightType() const { return LightType; }
	bool IsEnabled() const { return bEnabled; }
	bool IsShown(bool bGameView) const { return bVisible && !(bGameView && bHiddenInGame); }
	const FLightRenderData& GetRenderData() const { return RenderData; }
	const FVector& GetCullingSphereCenter() const { return CullingSphereCenter; }
	float GetCullingSphereRadius() const { return CullingSphereRadius; }

private:
	void UpdateSpotSphere(float Radius, float OuterAngle);
	FVector CullingSphereCenter{};
	float CullingSphereRadius = 0.0f;
	float SpotSphereOffset = 0.0f;
	float CachedSpotRadius = -1.0f;
	float CachedSpotOuterAngle = -1.0f;

	// UPointLightComponent* Component = nullptr;
	ULightComponent* Component = nullptr;
	ELightType LightType = ELightType::Point;
	FLightRenderData RenderData{};
	bool bEnabled = false;
	bool bVisible = true;
	bool bHiddenInGame = false;
};
