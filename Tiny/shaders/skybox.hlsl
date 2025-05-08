cbuffer ilCamera : register(b0)
{
    row_major float4x4 invViewProj;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 dir : TEXCOORD0;
};

VSOutput VS_Skybox(uint vid : SV_VertexID)
{
    static const float2 V[3] =
    {
        float2(-1, -1),
        float2(3, -1),
        float2(-1, 3)
    };
    float2 p = V[vid];
    
    VSOutput o;
    o.pos = float4(p, 0, 1);
    
    float4 worldPos = mul(float4(p, 1, 1), invViewProj);
    o.dir = worldPos.xyz;
    
    return o;
}

TextureCube skyTex : register(t0);
SamplerState skySamp : register(s0);

float4 PS_Skybox(VSOutput IN) : SV_TARGET
{
    return skyTex.Sample(skySamp, normalize(IN.dir));
}
