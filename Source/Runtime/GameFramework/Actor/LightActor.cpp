#include "EnginePCH.h"
#include "LightActor.h"
#include "Asset/AssetManager.h"
#include "Render/Texture2D.h"

#include "Component/PointLightComponent.h"
#include "Component/SpotLightComponent.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/AmbientLightComponent.h"

ALightActor::ALightActor()
{
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	SetRootComponent(BillboardComponent);

	// 아이콘용 메시/머티리얼. Plane은 FVertex 포맷이라 DefaultShader로 그대로 그려진다.
	// 라이트 아이콘 텍스처가 준비되면 Property 창에서 머티리얼 슬롯에 끼우면 된다.
	if (UTexture2D* SpotIcon = UAssetManager::Get().LoadTexture("Editor/Icon/SpotLight_64x.png"))
	{
		BillboardComponent->SetSprite(SpotIcon);
	}

	LightComponent = CreateDefaultSubobject<USpotLightComponent>("USpotLightComponent");
	LightComponent->SetupAttachment(BillboardComponent);
}

void ALightActor::OnPropertyChanged(const FString& PropertyName)
{
    Super::OnPropertyChanged(PropertyName);

    if (PropertyName == "LightType")
    {
        SetLightType(LightType);
    }
}

void ALightActor::SetLightType(ELightType InType)
{
    // 이미 같은 종류면 교체할 필요 없음
    if (LightComponent && LightComponent->GetLightType() == InType)
        return;

    ULightComponent* NewLight = nullptr;

    switch (InType)
    {
    case ELightType::Point:
        NewLight = CreateDefaultSubobject<UPointLightComponent>("UPointLightComponent");
        break;

    case ELightType::Spot:
        NewLight = CreateDefaultSubobject<USpotLightComponent>("USpotLightComponent");
        break;

    case ELightType::Directional:
        NewLight = CreateDefaultSubobject<UDirectionalLightComponent>("UDirectionalLightComponent");
        break;

    case ELightType::Ambient:
        NewLight = CreateDefaultSubobject<UAmbientLightComponent>("UAmbientLightComponent");
        break;

    default:
        return;
    }

    const bool bWasRegistered = LightComponent && LightComponent->IsRegistered();

    if (LightComponent)
    {
        // 기존 Proxy 제거
        LightComponent->UnregisterComponent();

        // 부착 관계와 Actor 소유 목록에서도 제거됨
        delete LightComponent;
    }

    LightComponent = NewLight;
    LightComponent->SetupAttachment(BillboardComponent);

    // 이미 월드에 등록된 광원을 교체하는 경우 새 Proxy 등록
    if (bWasRegistered)
        LightComponent->RegisterComponent();
}
