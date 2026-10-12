#pragma once

#include "Component/PointLightComponent.h"

class FLineBatcher;

class USpotLightComponent : public UPointLightComponent
{
	DECLARE_CLASS(USpotLightComponent, UPointLightComponent)

	REFLECT_START(USpotLightComponent)
		PROPERTY(InnerConeAngle)
		PROPERTY(OuterConeAngle)
		PROPERTY(Falloff)
	REFLECT_END()

public:
	USpotLightComponent()
	{
		AttenuationRadius = 5.0f;
		Intensity = 1.0f;
		LightColor = FVector4(1.0f, 1.0f, 0.6f, 1.0f);
	}
	virtual ~USpotLightComponent() override = default;
	ELightType GetLightType() const override { return ELightType::Spot; }
	// 광선 진행 방향. 위치는 GetWorldLocation()을 사용한다.
	FVector GetLightDirection() const
	{
		FTransform WorldTransform;
		WorldTransform.Rotation = GetWorldRotation();
		return WorldTransform.GetForward();
	}

	void DrawDebug(FLineBatcher* LineBatcher) const;

	float GetInnerConeAngle() const { return InnerConeAngle; }
	void SetInnerConeAngle(float InAngle) { InnerConeAngle = InAngle; OnPropertyChanged("InnerConeAngle"); }

	float GetOuterConeAngle() const { return OuterConeAngle; }
	void SetOuterConeAngle(float InAngle) { OuterConeAngle = InAngle; OnPropertyChanged("OuterConeAngle"); }

	float GetFalloff() const { return Falloff; }
	void SetFalloff(float InFalloff) { Falloff = InFalloff; OnPropertyChanged("Falloff"); }

private:
	// 원뿔 중심축 기준 반각(도). 
	float InnerConeAngle = 15.0f;
	float OuterConeAngle = 30.0f;

	// Inner에서 Outer로 가며 밝기가 떨어지는 정도.
	float Falloff = 1.0f;

};
