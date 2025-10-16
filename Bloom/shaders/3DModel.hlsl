#pragma pack_matrix(column_major)

[[vk::binding(0, 0)]]
cbuffer UniformBuffer
{
    float4x4 Model;
    float4x4 View;
    float4x4 Proj;
};

struct VertexInput
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(Proj, mul(View, mul(Model, input.Position)));
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
}

Texture2D r_Texture : register(t1);
SamplerState r_Sampler : register(s1);

void PShader(in VertexOutput input, out float4 outColor : SV_Target0)
{
    float4 texColor = r_Texture.Sample(r_Sampler, input.TexCoord);
    texColor *= input.Color;

    outColor = texColor;
}
