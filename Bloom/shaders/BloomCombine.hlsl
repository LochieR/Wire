#pragma pack_matrix(column_major)

struct VertexOutput
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD;
};

VertexOutput VShader(uint vertexID : SV_VertexID)
{
    const float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2( 3.0f, -1.0f),
        float2(-1.0f,  3.0f)
    };

    VertexOutput output;

    output.Position = float4(positions[vertexID], 0.0f, 1.0f);
    output.UV = output.Position.xy * 0.5f + 0.5f;

    return output;
}

[[vk::push_constant]]
cbuffer PushConstants
{
    float u_BloomStrength;
};

[[vk::binding(0, 0)]]
Texture2D<float4> r_Color;

[[vk::binding(1, 0)]]
Texture2D<float4> r_Bloom;

[[vk::binding(2, 0)]]
SamplerState r_Sampler;

float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

void PShader(in VertexOutput input, out float4 output : SV_Target0)
{
    float4 scene = r_Color.Sample(r_Sampler, input.UV);
    float4 bloom = r_Bloom.Sample(r_Sampler, input.UV);

    float4 combined = scene + bloom * u_BloomStrength;

    float3 toneMapped = ACESFilm(combined.rgb);
    toneMapped = pow(toneMapped, 1.0f / 2.2f);

    output = float4(toneMapped, 1.0f);
}
