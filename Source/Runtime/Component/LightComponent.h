#pragma once

#include "SceneComponent.h"
#include "Math/Vector4.h"

enum class ELightType : uint8
{
	Ambient,
	Directional,
	Point,
	Spot
};

class ULightComponent : public USceneComponent
{
	DECLARE_CLASS(ULightComponent, USceneComponent)
	REFLECT_START(ULightComponent)
		PROPERTY_TYPE(LightColor, Color)
		PROPERTY(Intensity)
		PROPERTY(bEnabled)
		PROPERTY(bShowLightBounds)
	REFLECT_END()
public:
	~ULightComponent() override { UnregisterComponent(); }
	virtual ELightType GetLightType() const = 0;
	const FVector& GetLightColor() const { return LightColor; }
	void SetLightColor(const FVector& InColor) { LightColor = InColor; OnPropertyChanged("LightColor"); }
	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; OnPropertyChanged("Intensity"); }
	bool IsEnabled() const { return bEnabled; }
	bool IsLightBoundsVisible() const { return bShowLightBounds; }
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; OnPropertyChanged("bEnabled"); }

	void OnPropertyChanged(const FString& PropertyName)
	{
		Super::OnPropertyChanged(PropertyName);

		if (PropertyName == "Intensity" && Intensity < 0.0f) {Intensity = 0.0f;}
	}


protected:
	bool ShouldCreateRenderState() const override { return true; }
	void CreateRenderState() override;
	void DestroyRenderState() override;

	FVector LightColor{1.0f, 1.0f, 1.0f};
	float Intensity = 1.0f;
	bool bEnabled = true;
	bool bShowLightBounds = true;
};
