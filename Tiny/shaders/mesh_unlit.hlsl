cbuffer WorldData : register(b0)
{
    matrix worldViewProj;
};

cbuffer MaterialParams : register(b1)
{
    float4 baseColor;
};

#ifdef TEXTURE_MAPPING
Texture2D diffuseTexture : register(t0);
SamplerState linearSampler : register(s0);
#endif

struct VSInput
{
    float3 position : POSITION;
#ifdef TEXTURE_MAPPING
    float2 uv : TEXCOORD;
#endif
};

struct PSInput
{
    float4 position : SV_POSITION;
#ifdef TEXTURE_MAPPING
    float2 uv : TEXCOORD;
#endif
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProj);

#ifdef TEXTURE_MAPPING
    output.uv = input.uv;
#endif

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color = baseColor;
#ifdef TEXTURE_MAPPING
    color *= diffuseTexture.Sample(linearSampler, input.uv);
#endif
    return color;
}