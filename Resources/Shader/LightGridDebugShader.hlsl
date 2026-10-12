cbuffer LightGridDebugConstants : register(b0)
{
    uint2 ViewSize;
    uint TileCountX;
    uint TileSize;

    uint MaxLightsPerTile;
    float HeatMapMaxCount;
    float DigitHeight;
    float ViewPadding;
};

cbuffer LightGridDebugFontConstants : register(b1)
{
    // 숫자 0~9의 UV 사각형: left, top, right, bottom
    float4 DigitUV[10];

    float2 AtlasSize;
    float DistanceRange;
    float FontPadding;
};

Buffer<uint> TileLightCount : register(t0);
Texture2D FontAtlas : register(t1);
SamplerState LinearSampler : register(s0);

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

float3 GetHeatMapColor(uint Count)
{
    if (Count == 0)
    {
        return float3(0.04f, 0.04f, 0.04f);
    }

    // 목록에 저장할 수 있는 최대치를 넘긴 타일
    if (Count > MaxLightsPerTile)
    {
        return float3(1.0f, 0.0f, 1.0f);
    }

    float Ratio = saturate(float(Count) / HeatMapMaxCount);

    const float3 Blue = float3(0.0f, 0.15f, 0.8f);
    const float3 Green = float3(0.0f, 0.75f, 0.2f);
    const float3 Red = float3(1.0f, 0.1f, 0.0f);

    if (Ratio < 0.5f)
    {
        return lerp(Blue, Green, Ratio * 2.0f);
    }

    return lerp(Green, Red, (Ratio - 0.5f) * 2.0f);
}

float Median(float3 Value)
{
    return max(min(Value.r, Value.g), min(max(Value.r, Value.g), Value.b));
}

float GetNumberOpacity(uint Count, float2 LocalPosition, float2 TileExtent)
{
    // 타일 경계에서 숫자를 띄우기 위한 표시용 여백
    const float Margin = 1.0f;
    const float CellWidthRatio = 0.6f;

    float2 AvailableSize = TileExtent - Margin * 2.0f;
    if (any(AvailableSize <= 0.0f))
    {
        return 0.0f;
    }

    uint DigitCount = 1;
    uint Divisor = 1;
    uint Remaining = Count;

    // 0도 한 자리로 표시하며, 가장 높은 자릿수의 제수를 구한다.
    while (Remaining >= 10)
    {
        Remaining /= 10;
        Divisor *= 10;
        ++DigitCount;
    }

    float CellHeight = min(DigitHeight, AvailableSize.y);
    float CellWidth = min(CellHeight * CellWidthRatio, AvailableSize.x / float(DigitCount));
    float NumberWidth = CellWidth * float(DigitCount);

    float NumberLeft = (TileExtent.x - NumberWidth) * 0.5f;
    float RelativeX = LocalPosition.x - NumberLeft;

    if (RelativeX < 0.0f || RelativeX >= NumberWidth)
    {
        return 0.0f;
    }

    uint DigitIndex = uint(RelativeX / CellWidth);
    for (uint Index = 0; Index < DigitIndex; ++Index)
    {
        Divisor /= 10;
    }

    uint Digit = (Count / Divisor) % 10;
    float4 UVRect = DigitUV[Digit];

    // atlas의 글자 사각형을 비율을 유지하면서 숫자 칸에 맞춘다.
    float2 AtlasGlyphSize = (UVRect.zw - UVRect.xy) * AtlasSize;
    float Scale = min(CellWidth / AtlasGlyphSize.x, CellHeight / AtlasGlyphSize.y);
    float2 GlyphSize = AtlasGlyphSize * Scale;

    float2 CellCenter = float2(NumberLeft + (float(DigitIndex) + 0.5f) * CellWidth, TileExtent.y * 0.5f);
    float2 GlyphPosition = LocalPosition - (CellCenter - GlyphSize * 0.5f);

    if (any(GlyphPosition < 0.0f) || any(GlyphPosition >= GlyphSize))
    {
        return 0.0f;
    }

    float2 UV = lerp(UVRect.xy, UVRect.zw, GlyphPosition / GlyphSize);
    float3 MSDF = FontAtlas.SampleLevel(LinearSampler, UV, 0).rgb;

    // atlas 픽셀에서 화면 픽셀로 변환되는 배율로 MSDF 경계를 계산한다.
    float ScreenPxRange = max(DistanceRange * Scale, 1.0f);
    float SignedDistance = Median(MSDF) - 0.5f;
    return saturate(SignedDistance * ScreenPxRange + 0.5f);
}

float4 mainPS(VSOutput Input) : SV_Target
{
    uint2 PixelPosition = uint2(Input.Position.xy);
    uint2 TilePosition = PixelPosition / TileSize;
    uint TileIndex = TilePosition.y * TileCountX + TilePosition.x;

    // 디버그 화면은 최대치로 자르기 전의 실제 개수를 표시한다.
    uint Count = TileLightCount[TileIndex];
    float3 Color = GetHeatMapColor(Count);

    uint2 TileOrigin = TilePosition * TileSize;
    uint2 LocalPixel = PixelPosition - TileOrigin;

    // 타일의 위쪽과 왼쪽에 1픽셀 경계선을 표시한다.
    if (LocalPixel.x == 0 || LocalPixel.y == 0)
    {
        Color *= 0.35f;
    }

    // 화면 끝에 있는 불완전한 타일도 실제 남은 크기를 사용한다.
    uint2 TileExtent = min(uint2(TileSize, TileSize), ViewSize - TileOrigin);
    float2 LocalPosition = Input.Position.xy - float2(TileOrigin);

    float NumberOpacity = GetNumberOpacity(Count, LocalPosition, float2(TileExtent));
    float ShadowOpacity = GetNumberOpacity(Count, LocalPosition - float2(1.0f, 1.0f), float2(TileExtent));

    // 라이트가 없는 타일은 배경을 유지하고 숫자 0만 표시한다.
    float Alpha = Count > 0 ? 0.25f : 0.0f;
    float3 PremultipliedColor = Color * Alpha;
    PremultipliedColor *= 1.0f - ShadowOpacity;
    Alpha = ShadowOpacity + Alpha * (1.0f - ShadowOpacity);
    PremultipliedColor = float3(1.0f, 1.0f, 1.0f) * NumberOpacity + PremultipliedColor * (1.0f - NumberOpacity);
    Alpha = NumberOpacity + Alpha * (1.0f - NumberOpacity);

    // 엔진 AlphaBlend의 SrcAlpha 곱셈에 맞춰 일반 알파 색상으로 반환한다.
    if (Alpha <= 0.0f)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    return float4(PremultipliedColor / Alpha, Alpha);
}
