#include "EnginePCH.h"
#include "Editor/Rendering/LightVisualizer.h"
#include "Asset/AssetManager.h"
#include "Component/LightComponent.h"
#include "Component/PointLightComponent.h"
#include "Component/SpotLightComponent.h"
#include "Render/LineBatcher.h"
#include "Engine/World.h"
#include "Engine/LightSceneProxy.h"
#include "Render/Mesh.h"
#include "Render/RenderCommand.h"
#include "Render/RenderResourceManager.h"
#include <cmath>

namespace
{
	struct FLightArrowData
	{
		FMatrix World;
		FMatrix ViewProj;
		FVector4 Color;
	};
	constexpr int32 CONE_SEGMENT_COUNT = 24;
	constexpr int32 CONE_SIDE_LINE_COUNT = 24;
	constexpr int32 CAP_ARC_SEGMENT_COUNT = 16;

	// Outer/Inner 각도와 광원으로부터의 거리 반경으로 제한되는 범위를 그린다.
	void AddWireCone(FLineBatcher* LineBatcher, const FVector& Apex, const FVector& Forward, const FVector& Right, const FVector& Up, float HalfAngleDegrees, float Range, const FVector4& Color)
	{
		const float HalfAngle = FMath::DegreesToRadians(HalfAngleDegrees);
		const float RingRadius = std::sin(HalfAngle) * Range;
		const FVector RingCenter = Apex + Forward * (std::cos(HalfAngle) * Range);

		// 원뿔 옆면과 반경 구가 만나는 원.
		for (int32 Index = 0; Index < CONE_SEGMENT_COUNT; ++Index)
		{
			const float AngleA = 2.0f * PI * Index / CONE_SEGMENT_COUNT;
			const float AngleB = 2.0f * PI * (Index + 1) / CONE_SEGMENT_COUNT;
			const FVector A = RingCenter + (Right * std::cos(AngleA) + Up * std::sin(AngleA)) * RingRadius;
			const FVector B = RingCenter + (Right * std::cos(AngleB) + Up * std::sin(AngleB)) * RingRadius;
			LineBatcher->AddLine(A, B, Color);
		}

		// 옆선은 거리 반경에서 끝나며, 끝면은 평면이 아닌 구면이다.
		for (int32 Index = 0; Index < CONE_SIDE_LINE_COUNT; ++Index)
		{
			const float Angle = 2.0f * PI * Index / CONE_SIDE_LINE_COUNT;
			const FVector Radial = Right * std::cos(Angle) + Up * std::sin(Angle);
			LineBatcher->AddLine(Apex, RingCenter + Radial * RingRadius, Color);

			// 네 방향의 호로 둥근 끝면을 표시한다.
			if (Index % (CONE_SIDE_LINE_COUNT / 4) == 0)
			{
				FVector Previous = Apex + Forward * Range;
				for (int32 Segment = 1; Segment <= CAP_ARC_SEGMENT_COUNT; ++Segment)
				{
					const float ArcAngle = HalfAngle * Segment / CAP_ARC_SEGMENT_COUNT;
					const FVector Point = Apex + (Forward * std::cos(ArcAngle) + Radial * std::sin(ArcAngle)) * Range;
					LineBatcher->AddLine(Previous, Point, Color);
					Previous = Point;
				}
			}
		}
	}
	void DrawPointLight(const UPointLightComponent& Light, FLineBatcher* LineBatcher)
	{
		const float AttenuationRadius = Light.GetAttenuationRadius();
		const FVector4 LightColor(Light.GetLightColor(), 1.0f);
		if (!LineBatcher || !(AttenuationRadius > 0.0f))
			return;

		const FVector Center = Light.GetWorldLocation();
		constexpr int32 SegmentCount = 32;

		// XY, XZ, YZ 평면의 원으로 구형 범위를 표시한다.
		for (int32 Index = 0; Index < SegmentCount; ++Index)
		{
			const float AngleA = 2.0f * PI * Index / SegmentCount;
			const float AngleB = 2.0f * PI * (Index + 1) / SegmentCount;
			const float CosA = std::cos(AngleA) * AttenuationRadius;
			const float SinA = std::sin(AngleA) * AttenuationRadius;
			const float CosB = std::cos(AngleB) * AttenuationRadius;
			const float SinB = std::sin(AngleB) * AttenuationRadius;

			LineBatcher->AddLine(Center + FVector(CosA, SinA, 0.0f),
				Center + FVector(CosB, SinB, 0.0f), LightColor);
			LineBatcher->AddLine(Center + FVector(CosA, 0.0f, SinA),
				Center + FVector(CosB, 0.0f, SinB), LightColor);
			LineBatcher->AddLine(Center + FVector(0.0f, CosA, SinA),
				Center + FVector(0.0f, CosB, SinB), LightColor);
		}
	}

	void DrawSpotLight(const USpotLightComponent& Light, FLineBatcher* LineBatcher)
	{
		const float AttenuationRadius = Light.GetAttenuationRadius();
		const float InnerConeAngle = Light.GetInnerConeAngle();
		const float OuterConeAngle = Light.GetOuterConeAngle();
		const FVector4 LightColor(Light.GetLightColor(), 1.0f);
		if (!LineBatcher || !(AttenuationRadius > 0.0f))
		{
			return;
		}

		FTransform WorldTransform;
		WorldTransform.Location = Light.GetWorldLocation();
		WorldTransform.Rotation = Light.GetWorldRotation();

		FVector Apex = WorldTransform.Location;
		FVector Forward = WorldTransform.GetForward();
		FVector Right = WorldTransform.GetRight();
		FVector Up = WorldTransform.GetUp();

		// 에디터에서 값을 직접 드래그하므로 범위를 신뢰할 수 없다.
		// 조명 데이터와 동일한 각도 범위를 적용한다.
		const float SafeOuterAngle = FMath::Clamp(OuterConeAngle, 0.0f, 89.0f);
		const float SafeInnerAngle = FMath::Clamp(InnerConeAngle, 0.0f, SafeOuterAngle);
		const float SafeRadius = AttenuationRadius;

		// 바깥 원뿔은 라이트 색 그대로, 안쪽은 구분되도록 흐리게
		AddWireCone(LineBatcher, Apex, Forward, Right, Up, SafeOuterAngle, SafeRadius, LightColor);

		FVector4 InnerColor(LightColor.X * 0.5f, LightColor.Y * 0.5f, LightColor.Z * 0.5f, 1.0f);

		AddWireCone(LineBatcher, Apex, Forward, Right, Up, SafeInnerAngle, SafeRadius, InnerColor);

		// Spot Light의 근사 구를 보기 위해서 그린다. (기능적으로 하는 역할은 없어서 나중에 지워도 된다.)
		if (!Light.GetWorld())
			return;
		for (const FLightSceneProxy* Proxy : Light.GetWorld()->GetScene().GetLights())
		{
			if (Proxy->GetComponent() != &Light)
				continue;
			const auto* Data = std::get_if<FSpotLightRenderData>(&Proxy->GetRenderData());
			if (!Data)
				return;

			const FVector Center = Proxy->GetCullingSphereCenter();
			const float Radius = Proxy->GetCullingSphereRadius();
			const FVector4 SphereColor(0.0f, 1.0f, 1.0f, 1.0f);
			constexpr int32 SphereSegments = 64;
			for (int32 Index = 0; Index < SphereSegments; ++Index)
			{
				const float A = 2.0f * PI * Index / SphereSegments;
				const float B = 2.0f * PI * (Index + 1) / SphereSegments;
				const float CA = std::cos(A) * Radius;
				const float SA = std::sin(A) * Radius;
				const float CB = std::cos(B) * Radius;
				const float SB = std::sin(B) * Radius;
				LineBatcher->AddLine(Center + FVector(CA, SA, 0.0f), Center + FVector(CB, SB, 0.0f), SphereColor);
				LineBatcher->AddLine(Center + FVector(CA, 0.0f, SA), Center + FVector(CB, 0.0f, SB), SphereColor);
				LineBatcher->AddLine(Center + FVector(0.0f, CA, SA), Center + FVector(0.0f, CB, SB), SphereColor);
			}
			LineBatcher->AddLine(Center, Data->Location, FVector4(1.0f, 0.0f, 1.0f, 1.0f));

			return;
		}
	}

}

void FLightVisualizer::Init()
{
	ArrowMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Arrow");
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/GizmoShader.hlsl");
	CB = RenderCommand::CreateConstantBuffer(sizeof(FLightArrowData));
	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void FLightVisualizer::Draw(ULightComponent* Light, const FMatrix& ViewProj, FLineBatcher* LineBatcher)
{
	if (!Light || !Light->IsEnabled() || !Light->IsLightBoundsVisible())
		return;

	switch (Light->GetLightType())
	{
	case ELightType::Directional:
		DrawDirectionArrow(Light->GetWorldLocation(), Light->GetWorldRotation(), ViewProj);
		break;
	case ELightType::Point:
		if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light))
			DrawPointLight(*Point, LineBatcher);
		break;
	case ELightType::Spot:
		if (USpotLightComponent* Spot = Cast<USpotLightComponent>(Light))
			DrawSpotLight(*Spot, LineBatcher);
		break;
	case ELightType::Ambient:
		break;
	}
}

void FLightVisualizer::DrawDirectionArrow(const FVector& Location, const FRotator& Rotation, const FMatrix& ViewProj)
{
	if (!ArrowMesh || !Shader || !CB)
		return;

	FTransform ArrowTransform;
	ArrowTransform.Rotation = Rotation;

	// Arrow 메시의 +Z 축을 광원의 로컬 +X 방향으로 맞춘다.
	// 원본 길이 1.5에 2배를 적용해 기존 표시 길이 3을 유지한다.
	constexpr float ArrowScale = 2.0f;
	const FVector Right = ArrowTransform.GetRight() * ArrowScale;
	const FVector Up = ArrowTransform.GetUp() * ArrowScale;
	const FVector Direction = ArrowTransform.GetForward() * ArrowScale;

	FLightArrowData Data{};
	Data.World = FMatrix::Identity;
	Data.World.M[0][0] = Right.X; Data.World.M[0][1] = Right.Y; Data.World.M[0][2] = Right.Z;
	Data.World.M[1][0] = Up.X; Data.World.M[1][1] = Up.Y; Data.World.M[1][2] = Up.Z;
	Data.World.M[2][0] = Direction.X; Data.World.M[2][1] = Direction.Y; Data.World.M[2][2] = Direction.Z;
	Data.World.M[3][0] = Location.X; Data.World.M[3][1] = Location.Y; Data.World.M[3][2] = Location.Z;
	Data.ViewProj = ViewProj;
	Data.Color = FVector4(0.6f, 0.6f, 0.6f, 1.0f);

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindMesh(ArrowMesh);
	RenderCommand::UpdateBufferData(CB.get(), &Data, sizeof(FLightArrowData));
	RenderCommand::BindConstantBuffer(0, CB.get(), EShaderBindFlagBits::Vertex);
	RenderCommand::DrawIndexed(ArrowMesh->IndexBuffer->GetIndexCount());
}

