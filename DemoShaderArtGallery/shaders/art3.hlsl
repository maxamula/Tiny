cbuffer ilShaderParams : register(b0)
{
    float2 iResolution;
    float iTime;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput o;
    float2 pos = float2((vertexID << 1) & 2, vertexID & 2) * 2.0f - 1.0f;
    o.position = float4(pos, 0.0f, 1.0f);
    o.uv = pos * 0.5f + 0.5f;
    return o;
}

float3 palette(float t)
{
    float3 a = float3(0.5, 0.5, 0.5);
    float3 b = float3(0.5, 0.5, 0.5);
    float3 c = float3(1.0, 1.0, 1.0);
    float3 d = float3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
    float2 fragCoord = IN.uv * iResolution;
    float2 uv = (fragCoord * 2.0 - iResolution) / iResolution.y;
    float2 uv0 = uv;
    float3 colSum = float3(0.0, 0.0, 0.0);

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        uv = frac(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));
        float3 col = palette(length(uv0) + i * 0.4 + iTime * 0.4);

        d = sin(d * 8.0 + iTime) / 8.0;
        d = abs(d);
        d = pow(0.01 / d, 1.2);

        colSum += col * d;
    }

    return float4(colSum, 1.0);
}