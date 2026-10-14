#pragma pack_matrix(row_major)

cbuffer FrameConstants : register(b0)
{
    matrix VP;
};

cbuffer ObjectConstants : register(b2)
{
    matrix World;
};

float4 mainVS(float3 position : POSITION) : SV_POSITION
{
    // UberLit과 같은 위치 계산을 사용한다.
    float3 worldPosition = mul(float4(position, 1.0f), World).xyz;
    return mul(float4(worldPosition, 1.0f), VP);
}

// 색상 타깃 없이 깊이만 기록한다. 현재 opaque에는 alpha clip이 없다.
void mainPS()
{
}
