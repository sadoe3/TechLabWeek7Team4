#include "EnginePCH.h"
#include "Component/SpotLightComponent.h"

void USpotLightComponent::OnPropertyChanged(const FString& PropertyName)
{
	Super::OnPropertyChanged(PropertyName);

	if (PropertyName == "InnerConeAngle" || PropertyName == "OuterConeAngle")
	{
		OuterConeAngle = FMath::Clamp(OuterConeAngle, 1.0f, 80.0f);
		InnerConeAngle = FMath::Clamp(InnerConeAngle, 1.0f, OuterConeAngle);
	}
	else if (PropertyName == "Falloff" && Falloff < 0.0f)
	{
		Falloff = 0.0f;
	}
}
