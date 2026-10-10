#pragma pack_matrix(row_major)

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
	float Range;
	float3 Position;
	float Padding;
};

struct SpotLightInfo
{
	float3 Color;
	float Range;
	float3 Position;
	float InnerAngle;
	float3 Direction;
	float OuterAngle;
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

float3 Diffuse(float3 Kd, float3 N, float3 L, float3 LightColor)
{
	float NdotL = saturate(dot(N, L));
	return Kd * LightColor * NdotL / Pi;
}

float3 Specular(float3 Ks, float3 R, float3 V, float3 LightColor, float Shininess)
{
	float RdotV = saturate(dot(R, V));
	return Ks * LightColor * pow(RdotV, Shininess) * (Shininess + 2.0f) / (2.0f * Pi);
}

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output;
	
	Output.WorldPosition = mul(float4(Input.Position, 1.0f), World).xyz;
	Output.Position = mul(float4(Output.WorldPosition, 1.0f), VP);
	Output.Normal = mul(float4(Input.Normal, 0.0f), World).xyz; // TODO: 비균등 스케일 고려 필요
	Output.Color = Input.Color;
	Output.UV = Input.UV;
	
	return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
	float4 TexColor = Texture.Sample(Sampler, Input.UV + UVOffset);
	float4 Albedo = TexColor * BaseColor;
	  
	float3 NormalDir = normalize(Input.Normal);
	float3 ReflectDir = reflect(LightDir, NormalDir);
	float3 ViewDir = normalize(CameraPosition - Input.WorldPosition);
	
	float3 FinalColor = float3(0.0f, 0.0f, 0.0f);
	
	// Ambient Light
	FinalColor += Albedo.rgb * AmbientColor;
	
	// Directional Light
	FinalColor += Diffuse(Albedo.rgb, NormalDir, -LightDir, LightColor);
	if (dot(NormalDir, -LightDir) > 0.0f)
	{
		FinalColor += Specular(float3(1.0f, 1.0f, 1.0f), ReflectDir, ViewDir, LightColor, 10.0f);
	}
	
	return float4(FinalColor.rgb, Albedo.a);
}
