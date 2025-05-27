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
    float CornerRadius : TEXCOORD1;
    float Fade : TEXCOORD2;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 LocalPosition : TEXCOORD0;
    float4 Color : COLOR;
    float CornerRadius : TEXCOORD1;
    float Fade : TEXCOORD2;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(ViewProjection, float4(input.WorldPosition, 1.0f));
    output.LocalPosition = input.LocalPosition;
    output.Color = input.Color;
    output.CornerRadius = input.CornerRadius;
    output.Fade = input.Fade;
}

void PShader(in VertexOutput input, out float4 outColor : SV_Target0)
{
    if (input.CornerRadius < 0.005f)
    {
        outColor = input.Color;
        return;
    }
    
    float2 pos = input.LocalPosition.xy;
    
    float2 q = abs(pos) - (1.0f - input.CornerRadius);
    float distanceToCorner = length(max(q, 0.0f)) - input.CornerRadius;
    
    float edgeFactor = 1.0f - smoothstep(-input.Fade, input.Fade, distanceToCorner);
    
    if (edgeFactor <= 0.0f)
        discard;
    
    float4 color = input.Color;
    color.a *= edgeFactor;
    
    outColor = color;
}