#pragma pack_matrix(column_major)

[[vk::push_constant]]
cbuffer PushConstants
{
    float4x4 ViewProjection;
};

Texture2D r_Textures[32] : register(t0);
SamplerState r_Sampler : register(s1);

struct VertexInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
    int FontIndex : TEXCOORD1;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
    int FontIndex : TEXCOORD1;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(ViewProjection, float4(input.Position.xyz, 1.0f));
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.FontIndex = input.FontIndex;
}

float ScreenPxRange(float2 texCoord, int index)
{
    const float range = 2.0f;
    int width, height, numMipLevels;
    r_Textures[index].GetDimensions(0u, width, height, numMipLevels);
    float2 unitRange = float2(range) / float2(float(width), float(height));
    float2 screenTexSize = float2(1.0f) / fwidth(texCoord);
    return max(0.5f * dot(unitRange, screenTexSize), 1.0f);
}

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void PShader(in VertexOutput input, out float4 outColor : SV_Target)
{    
    float2 texCoord = input.TexCoord;
    
    float3 msd = r_Textures[input.FontIndex].Sample(r_Sampler, texCoord).rgb;
    float sd = Median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPxRange(texCoord, input.FontIndex) * (sd - 0.5f);
    float opacity = clamp(screenPxDistance + 0.5f, 0.0f, 1.0f);
    
    if (opacity == 0.0f)
    {
        discard;
    }
    
    outColor = lerp(float4(0.0f), input.Color, opacity);
    
    if (outColor.a == 0.0f)
    {
        discard;
    }
}
