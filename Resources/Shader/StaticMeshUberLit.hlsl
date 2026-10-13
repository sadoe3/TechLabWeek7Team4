#pragma pack_matrix(row_major)

#define SHADING_MODEL_LAMBERT 0
#define SHADING_MODEL_PHONG 1
#define SHADING_MODEL_BLINN_PHONG 2
#define SHADING_MODEL_GOURAUD 3

#ifndef SHADING_MODEL
#define SHADING_MODEL SHADING_MODEL_BLINN_PHONG
#endif

#ifndef LIGHTING_ENABLED
#define LIGHTING_ENABLED 1
#endif

#define SPECULAR_NONE 0
#define SPECULAR_PHONG 1
#define SPECULAR_BLINN_PHONG 2

#if SHADING_MODEL == SHADING_MODEL_LAMBERT
	#define SPECULAR_MODEL SPECULAR_NONE
	#define PER_VERTEX 0
#elif SHADING_MODEL == SHADING_MODEL_PHONG
	#define SPECULAR_MODEL SPECULAR_PHONG
	#define PER_VERTEX 0
#elif SHADING_MODEL == SHADING_MODEL_BLINN_PHONG
	#define SPECULAR_MODEL SPECULAR_BLINN_PHONG
	#define PER_VERTEX 0
#elif SHADING_MODEL == SHADING_MODEL_GOURAUD
	#define SPECULAR_MODEL SPECULAR_BLINN_PHONG
	#define PER_VERTEX 1
#else
	#error "Invalid SHADING_MODEL"
#endif

#define MAX_DIRECTIONAL_LIGHT 8
#define MAX_POINT_LIGHT 8
#define MAX_SPOT_LIGHT 8

struct AmbientLightInfo
{
	float3 Color;
	float Intensity;
};

struct DirectionalLightInfo
{
	float3 Color;
	float Intensity;
	float3 Direction;
	float Padding;
};

struct PointLightInfo
{
	float3 Color;
	float Intensity;
	float3 Location;
	float AttenuationRadius;
};

struct SpotLightInfo
{
	float3 Color;
	float Intensity;
	float3 Location;
	float AttenuationRadius;
	float3 Direction;
	float InnerConeAngle;
	float OuterConeAngle;
	float Falloff;
	float2 Padding;
};

cbuffer FrameConstants : register(b0)
{
	matrix VP;
	float3 CameraPosition;
	uint NumDirectionalLights;
	uint NumPointLights;
	uint NumSpotLights;
	uint TileCountX; // 0이면 전체 라이트 순회
	uint MaxLightsPerTile;
	AmbientLightInfo AmbientLight;
	DirectionalLightInfo DirectionalLights[MAX_DIRECTIONAL_LIGHT];
	PointLightInfo PointLights[MAX_POINT_LIGHT];
	SpotLightInfo SpotLights[MAX_SPOT_LIGHT];
	uint TileSize;
	uint3 TilePadding;
};

cbuffer MaterialParams : register(b1)
{
	float4 BaseColor;
	float2 UVOffset;
	float2 MaterialParamsPadding;
};

cbuffer WorldConstants : register(b2)
{
	matrix World;
};

struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 UV : TEXCOORD;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float3 WorldPosition : TEXCOORD0;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 UV : TEXCOORD1;
};
Texture2D Texture : register(t0);
#if PER_VERTEX == 0 && LIGHTING_ENABLED == 1
Buffer<uint> TileLightCount : register(t8);
Buffer<uint> TileLightIndices : register(t9);
#endif
SamplerState Sampler : register(s0);

static const float Pi = 3.14159265359f;

float3 Diffuse(float3 Kd)
{
	return Kd / Pi;
}

float3 SpecularPhong(float3 Ks, float3 R, float3 V, float Shininess)
{
	float RdotV = saturate(dot(R, V));
	return Ks * pow(RdotV, Shininess) * (Shininess + 2.0f) / (2.0f * Pi);
}

float3 SpecularBlinnPhong(float3 Ks, float3 N, float3 H, float Shininess)
{
	float NdotH = saturate(dot(N, H));
	return Ks * pow(NdotH, Shininess) * (Shininess + 8.0f) / (8.0f * Pi);
}

float3 Specular(float3 Ks, float3 N, float3 L, float3 V, float Shininess)
{
#if SPECULAR_MODEL == SPECULAR_PHONG
	return SpecularPhong(Ks, reflect(L, N), V, Shininess);
#elif SPECULAR_MODEL == SPECULAR_BLINN_PHONG
	float3 H = normalize(L + V);
	return SpecularBlinnPhong(Ks, N, H, Shininess);
#endif
	return float3(0.0f, 0.0f, 0.0f);
}

// 컬링 반경에 도달하면 기여도가 0이 된다.
float CalculateDistanceAttenuation(float DistanceSquared, float Radius)
{
	float RadiusSquared = Radius * Radius;
	if (Radius <= 0.0f || DistanceSquared >= RadiusSquared)
		return 0.0f;

	float RadiusFade = saturate(1.0f - DistanceSquared / RadiusSquared);
	return RadiusFade * RadiusFade / (1.0f + DistanceSquared);
}

float CalculateSpotAngleAttenuation(float CosAngle, float InnerConeAngle, float OuterConeAngle, float Falloff)
{
	float CosOuter = cos(OuterConeAngle);
	float CosInner = cos(InnerConeAngle);
	// Falloff가 0이어도 바깥 원뿔 밖에는 빛이 닿지 않는다.
	if (CosAngle <= CosOuter)
		return 0.0f;

	// 두 각도가 같으면 전이 구간 없이 항상 원뿔 내부로 처리한다.
	if (CosInner == CosOuter)
		return 1.0f;

	float T = smoothstep(CosOuter, CosInner, CosAngle);
	return pow(T, Falloff);
}

float3 CalculateLighting(float3 Albedo, float3 Normal, float3 Position, float3 View, float2 ScreenPosition)
{
#if LIGHTING_ENABLED == 0
	return Albedo;
#endif
	
	float3 FinalColor = AmbientLight.Color * AmbientLight.Intensity * Albedo;
	
	for (uint i = 0; i < NumDirectionalLights; ++i)
	{
		DirectionalLightInfo Light = DirectionalLights[i];
		
		float3 LightDir = normalize(Light.Direction);
		float NdotL = saturate(dot(Normal, -LightDir));
		FinalColor += NdotL * Light.Color * Light.Intensity * Diffuse(Albedo);
		FinalColor += NdotL * Light.Color * Light.Intensity * Specular(float3(1.0f, 1.0f, 1.0f), Normal, -LightDir, View, 40.0f);
	}
	
	// Gouraud와 컬링 결과가 없는 뷰는 기존 전체 라이트를 순회한다.
	uint LightCount = NumPointLights + NumSpotLights;
	uint ListOffset = 0;
#if PER_VERTEX == 0 && LIGHTING_ENABLED == 1
	// CPU가 전달한 화면 타일 크기. SV_POSITION은 뷰포트 내부 픽셀 좌표다.
	if (TileCountX > 0)
	{
		uint2 Tile = uint2(ScreenPosition) / TileSize;
		uint TileIndex = Tile.y * TileCountX + Tile.x;
		LightCount = min(TileLightCount[TileIndex], MaxLightsPerTile);
		ListOffset = TileIndex * MaxLightsPerTile;
	}
#endif
	for (uint i = 0; i < LightCount; ++i)
	{
		uint LightIndex = i;
#if PER_VERTEX == 0 && LIGHTING_ENABLED == 1
		if (TileCountX > 0)
			LightIndex = TileLightIndices[ListOffset + i];
#endif
		// 컬링 입력은 Point 배열 다음 Spot 배열 순서다.
		if (LightIndex < NumPointLights)
		{
			PointLightInfo Light = PointLights[LightIndex];

			float3 LightDir = Light.Location - Position;
			float DistanceSquared = dot(LightDir, LightDir);
			float Attenuation = CalculateDistanceAttenuation(DistanceSquared, Light.AttenuationRadius);
			if (Attenuation <= 0.0f)
				continue;

			float Distance = sqrt(DistanceSquared);
			LightDir /= max(Distance, 0.0001f);

			float NdotL = saturate(dot(Normal, LightDir));
			FinalColor += NdotL * Light.Color * Light.Intensity * Attenuation * Diffuse(Albedo);
			FinalColor += NdotL * Light.Color * Light.Intensity * Attenuation * Specular(float3(1.0f, 1.0f, 1.0f), Normal, LightDir, View, 40.0f);
		}
		else
		{
			SpotLightInfo Light = SpotLights[LightIndex - NumPointLights];

			float3 LightDir = Light.Location - Position;
			float DistanceSquared = dot(LightDir, LightDir);
			float DistanceAttenuation = CalculateDistanceAttenuation(DistanceSquared, Light.AttenuationRadius);
			if (DistanceAttenuation <= 0.0f)
				continue;

			float Distance = sqrt(DistanceSquared);
			LightDir /= max(Distance, 0.0001f);

			float Cos = dot(-LightDir, normalize(Light.Direction));
			float AngleAttenuation = CalculateSpotAngleAttenuation(Cos, Light.InnerConeAngle, Light.OuterConeAngle, Light.Falloff);
			if (AngleAttenuation <= 0.0f)
				continue;

			float NdotL = saturate(dot(Normal, LightDir));
			FinalColor += NdotL * Light.Color * Light.Intensity * DistanceAttenuation * AngleAttenuation * Diffuse(Albedo);
			FinalColor += NdotL * Light.Color * Light.Intensity * DistanceAttenuation * AngleAttenuation * Specular(float3(1.0f, 1.0f, 1.0f), Normal, LightDir, View, 40.0f);
		}
	}

	return FinalColor;
}

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output;
	
	Output.WorldPosition = mul(float4(Input.Position, 1.0f), World).xyz;
	Output.Position = mul(float4(Output.WorldPosition, 1.0f), VP);
	Output.Normal = normalize(mul(float4(Input.Normal, 0.0f), World).xyz); // TODO: 비균등 스케일 고려 필요
	Output.UV = Input.UV;
	
#if PER_VERTEX == 1
	Output.Color = float4(CalculateLighting(float3(1.0f, 1.0f, 1.0f), Output.Normal, Output.WorldPosition, normalize(CameraPosition - Output.WorldPosition), float2(0.0f, 0.0f)), 1.0f);
#else
	Output.Color = Input.Color;
#endif
	
	return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
	float4 TexColor = Texture.Sample(Sampler, Input.UV + UVOffset);
	float4 Albedo = TexColor * BaseColor;
	  
#if PER_VERTEX == 1
	return Albedo * Input.Color;
#else
	float3 NormalDir = normalize(Input.Normal);
	float3 ViewDir = normalize(CameraPosition - Input.WorldPosition);
	float3 Color = CalculateLighting(Albedo.rgb, NormalDir, Input.WorldPosition, ViewDir, Input.Position.xy);
	
	return float4(Color, Albedo.a);
#endif
}
