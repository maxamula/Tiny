Texture2D diffuseTexture : register(t0);
SamplerState linearSampler : register(s0);

struct VSOutput
{
    noperspective float4 Position : SV_Position;
    noperspective float2 UV : TEXCOORD;
};

VSOutput VSMain(in uint VertexIdx : SV_VertexID)
{
    VSOutput output;
    float2 tex;
    float2 pos;

    switch (VertexIdx)
    {
        // First Triangle
        case 0:
            // Top-Left
            pos = float2(-1.0f, 1.0f);
            tex = float2(0.0f, 0.0f);
            break;
        case 1:
            // Bottom-Left
            pos = float2(-1.0f, -1.0f);
            tex = float2(0.0f, 1.0f);
            break;
        case 2:
            // Top-Right
            pos = float2(1.0f, 1.0f);
            tex = float2(1.0f, 0.0f);
            break;

        // Second Triangle
        case 3:
            // Top-Right
            pos = float2(1.0f, 1.0f);
            tex = float2(1.0f, 0.0f);
            break;
        case 4:
            // Bottom-Left
            pos = float2(-1.0f, -1.0f);
            tex = float2(0.0f, 1.0f);
            break;
        case 5:
            // Bottom-Right
            pos = float2(1.0f, -1.0f);
            tex = float2(1.0f, 1.0f);
            break;

        default:
            // Fallback position and UV (optional)
            pos = float2(0.0f, 0.0f);
            tex = float2(0.0f, 0.0f);
            break;
    }

    output.Position = float4(pos, 0.0f, 1.0f);
    output.UV = tex;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 color = diffuseTexture.Sample(linearSampler, input.UV);
    return color;
}