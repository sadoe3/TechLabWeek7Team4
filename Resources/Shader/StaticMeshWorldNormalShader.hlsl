#pragma pack_matrix(row_major)

#ifndef NORMAL_MAP_ENABLED
#define NORMAL_MAP_ENABLED 0
#endif

cbuffer ViewConstants : register(b0)
{
    matrix VP;
};

cbuffer WorldConstants : register(b2)
{
    matrix World;
    matrix NormalMatrix;
};

cbuffer MaterialParams : register(b1)
{
    float4 BaseColor;
    float3 SpecularColor;
    float Shininess;
    float2 UVOffset;
    float2 Padding;
};

struct WORLD_NORMAL_OUTPUT
{
    float4 BaseColorMetallic : SV_Target0;
    float4 WorldNormal : SV_Target1;
};

Texture2D ColorTexture : register(t0);
Texture2D NormalMap : register(t1);

SamplerState ColorSampler : register(s0);

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
    float3 Tangent : TANGENT;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldNormal : NORMAL;
    float2 UV : TEXCOORD1;
    float3 Tangent : TANGENT;
};

float3 CalculateNormalMapping(float3 SampledNormal, float3 Normal, float3 Tangent)
{
    float3 DecodedNormal = SampledNormal * 2.0f - 1.0f;
    float3 N = normalize(Normal);
    float3 T = normalize(Tangent - N * dot(Tangent, N));
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
	
    return normalize(mul(DecodedNormal, TBN));
}

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);
    Output.Position = mul(WorldPosition, VP);
    Output.WorldNormal = mul(float4(Input.Normal, 0.0f), NormalMatrix).xyz;
    Output.UV = Input.UV;
    Output.Tangent = mul(Input.Tangent, (float3x3)World);
    return Output;
}

WORLD_NORMAL_OUTPUT mainPS(PS_INPUT Input)
{
    WORLD_NORMAL_OUTPUT Output;

    float4 TextureColor = ColorTexture.Sample(ColorSampler, Input.UV + UVOffset);
    float3 LinearAlbedo = max(TextureColor.rgb, 0.0f) * BaseColor.rgb;
    float3 Normal = normalize(Input.WorldNormal);
    
    #if NORMAL_MAP_ENABLED == 1
    float3 SampledNormalMap = NormalMap.Sample(ColorSampler, Input.UV + UVOffset).rgb;
    Normal = CalculateNormalMapping(SampledNormalMap, Input.WorldNormal, Input.Tangent);
    #endif
    
    Output.BaseColorMetallic = float4(LinearAlbedo, 0.0f);
    Output.WorldNormal = float4(Normal, 1.0f);
    return Output;
}