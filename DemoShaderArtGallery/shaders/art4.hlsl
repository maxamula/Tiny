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
// globals
static float prm1;
static float2 bsMo;

// 2×2 rot
float2x2 rot2(in float a)
{
    float c = cos(a), s = sin(a);
    return float2x2(c, s,
                   -s, c);
}

// constant 3×3
static const float3x3 m3 = float3x3(
     0.33338, 0.56034, -0.71817,
    -0.87887, 0.32651, -0.15323,
     0.15162, 0.69596, 0.61339
) * 1.93;

float mag2(in float2 p)
{
    return dot(p, p);
}
float linstep(in float mn, in float mx, in float x)
{
    return clamp((x - mn) / (mx - mn), 0, 1);
}
float2 disp(in float t)
{
    return float2(sin(t * 0.22), cos(t * 0.175)) * 2;
}

float2 map(in float3 p)
{
    float3 p2 = p;
    p2.xy -= disp(p.z);
    p.xy = mul(p.xy, rot2(sin(p.z + iTime) * (0.1 + prm1 * 0.05) + iTime * 0.09));

    float cl = mag2(p2.xy);
    float d = 0;
    p *= 0.61;
    float z = 1, trk = 1, dspAmp = 0.1 + prm1 * 0.2;

    [unroll]
    for (int i = 0; i < 5; i++)
    {
        p += sin(p.zxy * 0.75 * trk + iTime * trk * 0.8) * dspAmp;
        d -= abs(dot(cos(p), sin(p.yzx))) * z;
        z *= 0.57;
        trk *= 1.4;
        p = mul(p, m3);
    }
    d = abs(d + prm1 * 3) + prm1 * 0.3 - 2.5 + bsMo.y;
    return float2(d + cl * 0.2 + 0.25, cl);
}

float4 renderVol(in float3 ro, in float3 rd, in float time)
{
    float4 rez = 0;
    float t = 1.5, fogT = 0;
    [loop]
    for (int i = 0; i < 130; i++)
    {
        if (rez.a > 0.99)
            break;

        float3 pos = ro + rd * t;
        float2 mpv = map(pos);
        float den = clamp(mpv.x - 0.3, 0, 1) * 1.12;
        float dn = clamp(mpv.x + 2, 0, 3);

        float4 col = 0;
        if (mpv.x > 0.6)
        {
            col = float4(
                sin(float3(5, 0.4, 0.2) + mpv.y * 0.1 + sin(pos.z * 0.4) * 0.5 + 1.8) * 0.5 + 0.5,
                0.08
            );
            col.rgb *= den * den * den;
            col.rgb *= linstep(4, -2.5, mpv.x) * 2.3;
            float dif = clamp((den - map(pos + 0.8).x) / 9, 0.001, 1)
                      + clamp((den - map(pos + 0.35).x) / 2.5, 0.001, 1);
            col.rgb *= den * (float3(0.005, 0.045, 0.075) + 1.5 * float3(0.033, 0.07, 0.03) * dif);
        }

        float fogC = exp(t * 0.2 - 2.2);
        col += float4(0.06, 0.11, 0.11, 0.1) * clamp(fogC - fogT, 0, 1);
        fogT = fogC;

        rez = rez + col * (1 - rez.a);
        t += clamp(0.5 - dn * dn * 0.05, 0.09, 0.3);
    }
    return clamp(rez, 0, 1);
}

float getsat(in float3 c)
{
    float mi = min(min(c.x, c.y), c.z);
    float ma = max(max(c.x, c.y), c.z);
    return (ma - mi) / (ma + 1e-7);
}

float3 iLerp(in float3 a, in float3 b, in float x)
{
    float3 ic = lerp(a, b, x) + float3(1e-6, 0, 0);
    float sd = abs(getsat(ic) - lerp(getsat(a), getsat(b), x));
    float3 dir = normalize(float3(
        2 * ic.x - ic.y - ic.z,
        2 * ic.y - ic.x - ic.z,
        2 * ic.z - ic.y - ic.x
    ));
    float lgt = dot(ic, float3(1, 1, 1));
    float ff = dot(dir, normalize(ic));
    ic += 1.5 * dir * sd * ff * lgt;
    return clamp(ic, 0, 1);
}

void mainImage(out float4 fragColor, in float2 fragCoord)
{
    float2 q = fragCoord / iResolution;
    float2 p = (fragCoord - 0.5 * iResolution) / iResolution.y;

    // replace mouse: simple oscillation in X/Y from time
    bsMo = float2(sin(iTime * 0.3), cos(iTime * 0.5)) * 0.5;

    float time = iTime * 3;
    float3 ro = float3(0, 0, time);
    ro += float3(sin(iTime) * 0.5, 0, 0);
    ro.xy += disp(ro.z) * 0.85;

    // camera setup
    float tgtDst = 3.5;
    float3 target = normalize(ro - float3(disp(time + tgtDst) * 0.85, time + tgtDst));
    float3 rightdir = normalize(cross(target, float3(0, 1, 0)));
    float3 updir = normalize(cross(rightdir, target));
    rightdir = normalize(cross(updir, target));

    float3 rd = normalize((p.x * rightdir + p.y * updir) - target);
    rd.xy = mul(rd.xy, rot2(-disp(time + 3.5).x * 0.2 + bsMo.x));

    prm1 = smoothstep(-0.4, 0.4, sin(iTime * 0.3));
    float4 scn = renderVol(ro, rd, time);

    float3 col = scn.rgb;
    col = iLerp(col.bgr, col.rgb, clamp(1 - prm1, 0.05, 1));
    col = pow(col, float3(0.55, 0.65, 0.6)) * float3(1, 0.97, 0.9);
    col *= pow(16 * q.x * q.y * (1 - q.x) * (1 - q.y), 0.12) * 0.7 + 0.3;

    fragColor = float4(col, 1);
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
    float2 fragCoord = IN.uv * iResolution;
    float4 outC;
    mainImage(outC, fragCoord);
    return outC;
}