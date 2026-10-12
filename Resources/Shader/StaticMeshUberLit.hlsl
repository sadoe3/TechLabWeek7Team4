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
	float2 FrameConstantsPadding;
	AmbientLightInfo AmbientLight;
	DirectionalLightInfo DirectionalLights[MAX_DIRECTIONAL_LIGHT];
	PointLightInfo PointLights[MAX_POINT_LIGHT];
	SpotLightInfo SpotLights[MAX_SPOT_LIGHT];
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

float3 CalculateLighting(float3 Albedo, float3 Normal, float3 Position, float3 View)
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
	
	for (uint i = 0; i < NumPointLights; ++i)
	{
		PointLightInfo Light = PointLights[i];
		
		float3 LightDir = Light.Location - Position;
		float Distance = length(LightDir);
		LightDir /= max(Distance, 0.0001f);
		
		float D = Distance / max(Light.AttenuationRadius, 0.0001f);
		float Attenuation = saturate(1.0f / (D * D));
		
		float NdotL = saturate(dot(Normal, LightDir));
		FinalColor += NdotL * Light.Color * Light.Intensity * Attenuation * Diffuse(Albedo);
		FinalColor += NdotL * Light.Color * Light.Intensity * Attenuation * Specular(float3(1.0f, 1.0f, 1.0f), Normal, LightDir, View, 40.0f);
	}
	
	for (uint i = 0; i < NumSpotLights; ++i)
	{
		SpotLightInfo Light = SpotLights[i];
		
		float3 LightDir = Light.Location - Position;
		float Distance = length(LightDir);
		LightDir /= max(Distance, 0.0001f);
		
		float D = Distance / max(Light.AttenuationRadius, 0.0001f);
		float DistanceAttenuation = saturate(1.0f / (D * D));
		
		float Cos = dot(-LightDir, normalize(Light.Direction));
		float CosOuter = cos(Light.OuterConeAngle);
		float CosInner = cos(Light.InnerConeAngle);
		float T = smoothstep(CosOuter, CosInner, Cos);
		float AngleAttenuation = pow(T, Light.Falloff);
		
		float NdotL = saturate(dot(Normal, LightDir));
		FinalColor += NdotL * Light.Color * Light.Intensity * DistanceAttenuation * AngleAttenuation * Diffuse(Albedo);
		FinalColor += NdotL * Light.Color * Light.Intensity * DistanceAttenuation * AngleAttenuation * Specular(float3(1.0f, 1.0f, 1.0f), Normal, LightDir, View, 40.0f);
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
	Output.Color = float4(CalculateLighting(float3(1.0f, 1.0f, 1.0f), Output.Normal, Output.WorldPosition, normalize(CameraPosition - Output.WorldPosition)), 1.0f);
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
	
	float3 Color = CalculateLighting(Albedo.rgb, NormalDir, Input.WorldPosition, ViewDir);
	
	return float4(Color, Albedo.a);
#endif
}
