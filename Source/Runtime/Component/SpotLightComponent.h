#pragma once

#include "Component/PointLightComponent.h"

class USpotLightComponent : public UPointLightComponent
{
	DECLARE_CLASS(USpotLightComponent, UPointLightComponent)

	REFLECT_START(USpotLightComponent)
		PROPERTY_RANGE(InnerConeAngle, 1.0f, 80.0f)
		PROPERTY_RANGE(OuterConeAngle, 1.0f, 80.0f)
	REFLECT_END()

public:
	USpotLightComponent()
	{
		AttenuationRadius = 5.0f;
		Intensity = 1.0f;
		LightColor = FVector(1.0f, 1.0f, 0.6f);
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

	void OnPropertyChanged(const FString& PropertyName) override;

	float GetInnerConeAngle() const { return InnerConeAngle; }
	void SetInnerConeAngle(float InAngle) { InnerConeAngle = InAngle; OnPropertyChanged("InnerConeAngle"); }

	float GetOuterConeAngle() const { return OuterConeAngle; }
	void SetOuterConeAngle(float InAngle) { OuterConeAngle = InAngle; OnPropertyChanged("OuterConeAngle"); }

private:
	// 원뿔 중심축 기준 반각(도). 
	float InnerConeAngle = 15.0f;
	float OuterConeAngle = 30.0f;
};
