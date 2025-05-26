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
    int TextureIndex : TEXCOORD1;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
    int TextureIndex : TEXCOORD1;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(ViewProjection, float4(input.Position.xyz, 1.0f));
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.TextureIndex = input.TextureIndex;
}

void PShader(in VertexOutput input, out float4 outColor : SV_Target)
{
    float4 texColor = r_Textures[input.TextureIndex].Sample(r_Sampler, input.TexCoord);
    texColor *= input.Color;
    
    outColor = texColor;
}
