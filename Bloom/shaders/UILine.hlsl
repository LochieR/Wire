#pragma pack_matrix(column_major)

[[vk::push_constant]]
cbuffer PushConstants
{
    float4x4 ViewProjection;
};

struct VertexInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(ViewProjection, float4(input.Position, 1.0f));
    output.Color = input.Color;
}

void PShader(in VertexOutput input, out float4 outColor : SV_Target0)
{
    outColor = input.Color;
}
