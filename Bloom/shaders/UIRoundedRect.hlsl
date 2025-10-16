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
    float2 RectSize : TEXCOORD1;
    float4 Color : COLOR;
    float CornerRadius : TEXCOORD2;
    uint CornerFlags : TEXCOORD3;
    float Fade : TEXCOORD4;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 LocalPosition : TEXCOORD0;
    float2 RectSize : TEXCOORD1;
    float4 Color : COLOR;
    float CornerRadius : TEXCOORD2;
    uint CornerFlags : TEXCOORD3;
    float Fade : TEXCOORD4;
};

void VShader(in VertexInput input, out VertexOutput output)
{
    output.Position = mul(ViewProjection, float4(input.WorldPosition, 1.0f));
    output.LocalPosition = input.LocalPosition;
    output.RectSize = input.RectSize;
    output.Color = input.Color;
    output.CornerRadius = input.CornerRadius;
    output.CornerFlags = input.CornerFlags;
    output.Fade = input.Fade;
}

void PShader(in VertexOutput input, out float4 outColor : SV_Target0)
{
    if (input.CornerRadius < 0.005f || input.CornerFlags == 0)
    {
        outColor = input.Color;
        return;
    }
    
    const uint tl = 1 << 0;
    const uint tr = 1 << 1;
    const uint bl = 1 << 2;
    const uint br = 1 << 3;
    
    bool topLeft = (input.CornerFlags & tl) != 0;
    bool topRight = (input.CornerFlags & tr) != 0;
    bool bottomLeft = (input.CornerFlags & bl) != 0;
    bool bottomRight = (input.CornerFlags & br) != 0;

    float2 halfSize = input.RectSize * 0.5f;
    float2 pixelPos = input.LocalPosition.xy * halfSize;

    float2 cornerRadius = float2(input.CornerRadius, input.CornerRadius);
    float2 innerRect = halfSize - cornerRadius;
    
    bool inTop = pixelPos.y > innerRect.y;
    bool inBottom = pixelPos.y < -innerRect.y;
    bool inRight = pixelPos.x > innerRect.x;
    bool inLeft = pixelPos.x < -innerRect.x;
    
    float distanceToCorner = -input.CornerRadius;

    if (inTop && inLeft && topLeft)
    {
        float2 q = pixelPos - float2(-innerRect.x, innerRect.y);
        distanceToCorner = length(max(abs(q) - cornerRadius, 0.0f)) - input.CornerRadius;
    }
    else if (inTop && inRight && topRight)
    {
        float2 q = pixelPos - float2(innerRect.x, innerRect.y);
        distanceToCorner = length(max(abs(q) - cornerRadius, 0.0f)) - input.CornerRadius;
    }
    else if (inBottom && inLeft && bottomLeft)
    {
        float2 q = pixelPos - float2(-innerRect.x, -innerRect.y);
        distanceToCorner = length(max(abs(q) - cornerRadius, 0.0f)) - input.CornerRadius;
    }
    else if (inBottom && inRight && bottomRight)
    {
        float2 q = pixelPos - float2(innerRect.x, -innerRect.y);
        distanceToCorner = length(max(abs(q) - cornerRadius, 0.0f)) - input.CornerRadius;
    }

    float edgeFactor = 1.0f - smoothstep(-input.Fade, input.Fade, distanceToCorner);

    if (edgeFactor <= 0.0f)
        discard;

    float4 color = input.Color;
    color.a *= edgeFactor;

    outColor = color;
}

/*



*/