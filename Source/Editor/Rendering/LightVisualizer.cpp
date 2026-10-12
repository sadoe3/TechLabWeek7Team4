#include "EnginePCH.h"
#include "Editor/Rendering/LightVisualizer.h"
#include "Asset/AssetManager.h"
#include "Component/LightComponent.h"
#include "Component/PointLightComponent.h"
#include "Component/SpotLightComponent.h"
#include "Render/LineBatcher.h"
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
	// 밑면 원의 분할 수. 너무 높이면 라인 배처 정점을 빨리 소모한다.
	constexpr int32 CONE_SEGMENT_COUNT = 24;

	// 꼭짓점에서 밑면으로 뻗는 옆선 개수
	constexpr int32 CONE_SIDE_LINE_COUNT = 24;

	// Apex에서 Forward 방향으로 Range만큼 뻗은 원뿔을 와이어로 그린다.
	// Right/Up은 Forward에 직교하는 밑면 평면의 축이다.
	void AddWireCone(
		FLineBatcher* LineBatcher,
		const FVector& Apex,
		const FVector& Forward,
		const FVector& Right,
		const FVector& Up,
		float HalfAngleDegrees,
		float Range,
		const FVector4& Color)
	{
		const float HalfAngle = FMath::DegreesToRadians(HalfAngleDegrees);
		const float BaseRadius = std::tanf(HalfAngle) * Range;

		const FVector BaseCenter = Apex + Forward * Range;

		// 밑면 원
		FVector PreviousPoint;
		FVector FirstPoint;

		for (int32 i = 0; i < CONE_SEGMENT_COUNT; ++i)
		{
			const float Angle = (2.0f * PI * i) / CONE_SEGMENT_COUNT;

			const FVector Point =
				BaseCenter +
				Right * (std::cosf(Angle) * BaseRadius) +
				Up * (std::sinf(Angle) * BaseRadius);

			if (i == 0)
			{
				FirstPoint = Point;
			}
			else
			{
				LineBatcher->AddLine(PreviousPoint, Point, Color);
			}

			PreviousPoint = Point;
		}

		// 마지막 점과 첫 점을 이어 원을 닫는다
		LineBatcher->AddLine(PreviousPoint, FirstPoint, Color);

		// 옆선
		for (int32 i = 0; i < CONE_SIDE_LINE_COUNT; ++i)
		{
			const float Angle = (2.0f * PI * i) / CONE_SIDE_LINE_COUNT;

			const FVector Point =
				BaseCenter +
				Right * (std::cosf(Angle) * BaseRadius) +
				Up * (std::sinf(Angle) * BaseRadius);

			LineBatcher->AddLine(Apex, Point, Color);
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
		if (LineBatcher == nullptr)
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

		// 컴포넌트의 각도 제한과 같은 범위로 그린다.
		const float SafeOuterAngle = FMath::Clamp(OuterConeAngle, 1.0f, 80.0f);
		const float SafeInnerAngle = FMath::Clamp(InnerConeAngle, 1.0f, SafeOuterAngle);
		const float SafeRadius = (AttenuationRadius > 0.0f) ? AttenuationRadius : 0.0f;

		// 바깥 원뿔은 라이트 색 그대로, 안쪽은 구분되도록 흐리게
		AddWireCone(LineBatcher, Apex, Forward, Right, Up, SafeOuterAngle, SafeRadius, LightColor);

		FVector4 InnerColor(LightColor.X * 0.5f, LightColor.Y * 0.5f, LightColor.Z * 0.5f, 1.0f);

		AddWireCone(LineBatcher, Apex, Forward, Right, Up, SafeInnerAngle, SafeRadius, InnerColor);
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

