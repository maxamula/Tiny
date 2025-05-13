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


float4 PSMain(VSOutput IN) : SV_TARGET
{
    float2 fragCoord = IN.uv * iResolution;
    float2 I = fragCoord;

    float t = iTime;
    float z = 0.0;
    float d;
    float4 O = float4(0.0, 0.0, 0.0, 0.0);

    [unroll]
    for (int iter = 0; iter < 100; ++iter)
    {
        float3 p = z * normalize(float3(I + I, 0.0)
                                 - float3(iResolution.x, iResolution.y, iResolution.y));
        p.z -= t;

        d = 1.0;
        while (d < 9.0)
        {
            p += cos(p.yzx * d + z * 0.2) / d;
            d /= 0.7;
        }

        d = 0.02 + 0.1 * abs(3.0 - length(p.xy));
        z += d;

        O += (cos(z + t + float4(6.0, 1.0, 2.0, 3.0)) + 1.0) / d;
    }

    O = tanh(O / 3000.0);

    return O;
}