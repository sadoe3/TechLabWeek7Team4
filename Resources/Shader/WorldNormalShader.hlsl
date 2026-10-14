Texture2D<float4> WorldNormalTexture : register(t0);

struct VSOutput
{
    float4 Position : SV_Position;
};

VSOutput mainVS(uint VertexID : SV_VertexID)
{
    static const float2 Positions[6] =
    {
        float2(-1, 1),
        float2(1, 1),
        float2(-1, -1),

        float2(-1, -1),
        float2(1, 1),
        float2(1, -1)
    };

    VSOutput Output;
    Output.Position = float4(Positions[VertexID], 0, 1);
    return Output;
}

float4 mainPS(VSOutput Input) : SV_Target
{
    int2 PixelPosition = int2(Input.Position.xy);
    float3 Normal = WorldNormalTexture.Load(int3(PixelPosition, 0)).xyz;
    
    if (length(Normal) <= 1e-6f)
    {
        return float4(0, 0, 0, 0);
    }
    
    Normal = normalize(Normal);
    Normal = Normal * 0.5f + 0.5f;
    return float4(Normal, 1);
}