#pragma once

#include "GameFramework/Actor.h"
#include "Component/BillboardComponent.h"
#include "Component/SpotLightComponent.h"

class ASpotLightActor : public AActor
{
	DECLARE_CLASS(ASpotLightActor, AActor)
	REFLECT_START(ASpotLightActor)
	REFLECT_END()

public:
	ASpotLightActor();
	USpotLightComponent* GetSpotLightComponent() const { return SpotLightComponent; }

private:
	UBillboardComponent* BillboardComponent = nullptr;
	USpotLightComponent* SpotLightComponent = nullptr;
};
