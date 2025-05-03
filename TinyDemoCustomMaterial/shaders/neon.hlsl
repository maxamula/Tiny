cbuffer ilShaderParams : register(b0)
{
    float2 iResolution; // (width, height)
    float iTime; // seconds since start
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0; // in [0,1]
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput o;
    float2 pos = float2((vertexID << 1) & 2, vertexID & 2) * 2.0f - 1.0f;
    o.position = float4(pos, 0.0f, 1.0f);
    o.uv = pos * 0.5f + 0.5f; // map from [-1,1] to [0,1]
    return o;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
    float2 fragCoord = IN.uv * iResolution;

    float2 p = (2.0f * fragCoord - iResolution) / iResolution.y;
    const float tau = 6.28318530718f;
    float a = atan2(p.x, p.y);
    float r = length(p) * 0.75f;
    float2 uv = float2(a / tau, r);
    

    float xCol = (uv.x - (iTime / 3.0f)) * 3.0f;
    xCol = fmod(xCol, 3.0f);
    float3 horColour = float3(0.25f, 0.25f, 0.25f);

    if (xCol < 1.0f)
    {
        horColour.r += 1.0f - xCol;
        horColour.g += xCol;
    }
    else if (xCol < 2.0f)
    {
        xCol -= 1.0f;
        horColour.g += 1.0f - xCol;
        horColour.b += xCol;
    }
    else
    {
        xCol -= 2.0f;
        horColour.b += 1.0f - xCol;
        horColour.r += xCol;
    }

    uv = uv * 2.0f - 1.0f;
    float flicker = clamp(floor(5.0f + 10.0f * cos(iTime)), 0.0f, 10.0f);
    float beamWidth = (0.7f + 0.5f * cos(uv.x * 10.0f * tau * 0.15f * 3))
                    * abs(1.0f / (30.0f * uv.y));
    float3 horBeam = float3(beamWidth, beamWidth, beamWidth);

    return float4(horBeam * horColour, 1.0f);
}
