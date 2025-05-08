Texture2D diffuseTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer ilWorld : register(b0)
{
    matrix worldViewProj;
};

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProj);
    output.uv = input.uv;
    output.color = float3(output.uv.x, output.uv.y, 1.0f); // Default color (white)
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
   float4 colOnTex = diffuseTexture.Sample(linearSampler, input.uv);  
   return float4(input.color.xy, colOnTex.x, 1.0f);

}