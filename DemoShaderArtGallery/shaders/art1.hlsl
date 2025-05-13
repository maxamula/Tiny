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
    float2 u = fragCoord;
    float2 v = iResolution;

    u = 0.2 * (u + u - v) / v.y;

    float4 o = float4(1.0, 2.0, 3.0, 0.0);
    float4 z = o;

    float a = 0.5;
    float t = iTime;

    for (float i = 0.0; ++i < 19.0;)
    {
        float4 cosZt = cos(z + t);
        float angle = 1.5 * u / (0.5 - dot(u, u)) - 9.0 * u.yx + t;
        float denom = length((1.0 + i * dot(v, v)) * sin(angle));
        o += (1.0 + cosZt) / denom;

        v = cos(++t - 7.0 * u * pow(a += 0.03, i)) - 5.0 * u;

        float f = i + 0.02 * t;
        float4 ang4 = cos(float4(f, f, f, f) - float4(0.0, 11.0, 33.0, 0.0));
        float2x2 rotMat = float2x2(ang4.x, ang4.y,
                                   ang4.z, ang4.w);

        u = mul(u, rotMat);

        float duFactor = dot(u, u);
        float2 deltaU =
              tanh(40.0 * duFactor * cos(100.0 * u.yx + t)) / 200.0
            + 0.2 * a * u
            + cos(4.0 / exp(dot(o, o) / 100.0) + t) / 300.0;

        u += deltaU;
    }

    o = 25.6 / (min(o, 13.0) + 164.0 / o)
      - dot(u, u) / 250.0;

    return o;
}