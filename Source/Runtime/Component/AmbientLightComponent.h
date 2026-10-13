#pragma once

#include "Component/LightComponent.h"

// 위치/방향/거리 감쇠 없이 일정한 주변광을 제공한다.
class UAmbientLightComponent : public ULightComponent
{
	DECLARE_CLASS(UAmbientLightComponent, ULightComponent)
	REFLECT_START(UAmbientLightComponent)
	REFLECT_END()

public:
	UAmbientLightComponent() { Intensity = 0.1f; }
	ELightType GetLightType() const override { return ELightType::Ambient; }
};
