#pragma once

#include "Component/LightComponent.h"
#include "Math/Vector4.h"

class UPointLightComponent : public ULightComponent
{
	DECLARE_CLASS(UPointLightComponent, ULightComponent)
	REFLECT_START(UPointLightComponent)
		PROPERTY(AttenuationRadius)
	REFLECT_END()

public:
	UPointLightComponent() { Intensity = 8.0f; }
	~UPointLightComponent() override { UnregisterComponent(); }
	ELightType GetLightType() const override { return ELightType::Point; }
	void DrawDebug(FLineBatcher* LineBatcher) const override;
	float GetAttenuationRadius() const { return AttenuationRadius; }
	void SetAttenuationRadius(float InRadius) { AttenuationRadius = InRadius; OnPropertyChanged("AttenuationRadius"); }

protected:
	// 위치는 USceneComponent의 월드 위치를 사용한다.
	float AttenuationRadius = 10.0f;
};
