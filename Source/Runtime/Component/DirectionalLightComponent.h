#pragma once

#include "Component/LightComponent.h"

// 모든 위치에서 같은 방향으로 진행하며 거리 감쇠가 없는 조명.
class UDirectionalLightComponent : public ULightComponent
{
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	REFLECT_START(UDirectionalLightComponent)
	REFLECT_END()

public:
	ELightType GetLightType() const override { return ELightType::Directional; }
	// 로컬 +X를 광선 진행 방향으로 사용한다. 조명 식의 L은 이 방향의 반대다.
	FVector GetLightDirection() const
	{
		FTransform WorldTransform;
		WorldTransform.Rotation = GetWorldRotation();
		return WorldTransform.GetForward();
	}
};
