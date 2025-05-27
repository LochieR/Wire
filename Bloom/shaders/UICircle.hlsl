#pragma pack_matrix(column_major)

[[vk::push_constant]]
cbuffer PushConstants
{
    float4x4 ViewProjection;
};

struct VertexInput
{
    float3 WorldPosition : POSITION;
    float3 LocalPosition : TEXCOORD0;
    float4 Color : COLOR;
    float Thickness : TEXCOORD1;
    float Fade : TEXCOORD2;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 LocalPosition : TEXCOORD0;
    float4 Color : COLOR;
    float Thickness : TEXCOORD1;
    float Fade : TEXCOORD2;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(ViewProjection, float4(input.WorldPosition, 1.0f));
    output.LocalPosition = input.LocalPosition;
    output.Color = input.Color;
    output.Thickness = input.Thickness;
    output.Fade = input.Fade;
}

void PShader(in VertexOutput input, out float4 outColor : SV_Target0)
{
    float dist = 1.0f - length(input.LocalPosition);
    float circle = smoothstep(0.0f, input.Fade, dist);
    circle *= smoothstep(input.Thickness + input.Fade, input.Thickness, dist);
    
    if (circle == 0.0f)
        discard;
    
    float4 color = input.Color;
    color.a *= circle;
    
    outColor = color;
}
