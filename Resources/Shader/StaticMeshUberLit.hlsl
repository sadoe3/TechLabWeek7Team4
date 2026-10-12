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

#define MAX_DIRECTIONAL_LIGHT 4
#define MAX_POINT_LIGHT 4
#define MAX_SPOT_LIGHT 4

struct AmbientLightInfo
{
	float3 Color;
	float Padding;
};

struct DirectionalLightInfo
{
	float3 Color;
	float Padding1;
	float3 Direction;
	float Padding2;
};

struct PointLightInfo
{
	float3 Color;
	float AttenuationRadius;
	float3 Position;
	float Padding;
};

struct SpotLightInfo
{
	float3 Color;
	float AttenuationRadius;
	float3 Position;
	float InnerConeAngle;
	float3 Direction;
	float OuterConeAngle;
};

cbuffer FrameConstants : register(b0)
{
	matrix VP;
	float3 CameraPosition;
	float FrameConstantsPadding;
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

static const float3 LightDir = normalize(float3(0.5f, 0.5f, -1.0f));
static const float3 LightColor = float3(1.0f, 1.0f, 1.0f);
static const float3 AmbientColor = float3(0.5f, 0.5f, 0.5f);

float3 Diffuse(float3 Kd)
{
	return Kd / Pi;
}

float3 SpecularPhong(float3 Ks, float3 R, float3 V, float Shininess)
{
	float RdotV = saturate(dot(R, V));
	return Ks * pow(RdotV, Shininess) * (Shininess + 2.0f) / (2.0f * Pi);
}

float3 SpecularBlinnPhong(float3 Ks, float3 N, float3 L, float3 V, float Shininess)
{
	float3 H = normalize(L + V);
	float NdotH = saturate(dot(N, H));
	return Ks * pow(NdotH, Shininess) * (Shininess + 8.0f) / (8.0f * Pi);
}

float3 CalculateLighting(float3 Albedo, float3 Normal, float3 Position, float3 View)
{
	float3 FinalColor = Albedo * AmbientColor;
	
#if LIGHTING_ENABLED == 0
	return FinalColor;
#endif
	
	float NdotL = saturate(dot(Normal, -LightDir));
	FinalColor += NdotL * LightColor * Diffuse(Albedo);
#if SPECULAR_MODEL == SPECULAR_PHONG
	FinalColor += NdotL * LightColor * SpecularPhong(float3(1.0f, 1.0f, 1.0f), reflect(LightDir, Normal), View, 10.0f);
#elif SPECULAR_MODEL == SPECULAR_BLINN_PHONG
	FinalColor += NdotL * LightColor * SpecularBlinnPhong(float3(1.0f, 1.0f, 1.0f), Normal, -LightDir, View, 40.0f);
#endif
	
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
