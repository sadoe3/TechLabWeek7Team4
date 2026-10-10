#pragma pack_matrix(row_major)

#define MAX_AMBIENT_LIGHT 4
#define MAX_DIRECTIONAL_LIGHT 4
#define MAX_POINT_LIGHT 4
#define MAX_SPOT_LIGHT 4

struct AmbientLightInfo
{
    float3 Color;
};

struct DirectionalLightInfo
{
    float3 Color;
    float3 Direction;
};

struct PointLightInfo
{
    float3 Color;
    float3 Position;
    float Range;
};

struct SpotLightInfo
{
    float3 Color;
    float3 Position;
    float3 Direction;
    float Range;
    float InnerAngle;
    float OuterAngle;
};

cbuffer FrameConstants : register(b0)
{
	matrix VP;
	AmbientLightInfo AmbientLights[MAX_AMBIENT_LIGHT];
	DirectionalLightInfo DirectionalLights[MAX_DIRECTIONAL_LIGHT];
	PointLightInfo PointLights[MAX_POINT_LIGHT];
	SpotLightInfo SpotLights[MAX_SPOT_LIGHT];
};

cbuffer MaterialParams : register(b1)
{
	float4 BaseColor;
	float2 UVOffset;
	float2 Padding;
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
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
};

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

static const float3 LightDir = normalize(float3(0.5f, 0.5f, -1.0f));
static const float3 LightColor = float3(0.5f, 0.5f, 0.5f);
static const float3 AmbientColor = float3(0.5f, 0.5f, 0.5f);

float3 Diffuse(float3 Albedo, float3 N, float3 L, float3 LightColor)
{
    float NdotL = saturate(dot(N, L));
    return Albedo * LightColor * NdotL;
}

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output;
	
	Output.Position = mul(mul(float4(Input.Position, 1.0f), World), VP);
    Output.Normal = mul(float4(Input.Normal, 0.0f), World).xyz;
    Output.Color = Input.Color;
    Output.UV = Input.UV;
	
	return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float4 TexColor = Texture.Sample(Sampler, Input.UV + UVOffset);
    float4 Albedo = TexColor * BaseColor;
	
    float3 Normal = normalize(Input.Normal);
	
    float3 FinalColor = float3(0.0f, 0.0f, 0.0f);
	
    FinalColor += Diffuse(Albedo.rgb, Normal, -LightDir, LightColor);
	
    return float4(FinalColor.rgb, Albedo.a);
}
