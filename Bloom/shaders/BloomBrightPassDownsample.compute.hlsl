#pragma pack_matrix(column_major)

[[vk::push_constant]]
cbuffer PushConstants
{
    int2 u_SrcSize;
    int2 u_DestSize;
    float u_Threshold;
    float u_Intensity;
};

[[vk::binding(0, 0)]]
Texture2D<float4> r_InputImage;

[[vk::binding(1, 0)]]
SamplerState r_Sampler;

[[vk::binding(2, 0)]]
RWTexture2D<float4> r_OutputImage;

[numthreads(16, 16, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
    int2 coord = int2(DTid.xy);
    
    if (coord.x >= u_DestSize.x || coord.y >= u_DestSize.y)
        return;
    
    int2 srcCoord = coord * 2;

    float2 uv = (srcCoord + 1.5f) / float2(u_SrcSize);
    float2 texelSize = 1.0f / float2(u_SrcSize);

    // . . . . . . .
    // . A . B . C .
    // . . D . E . .
    // . F . G . H .
    // . . I . J . .
    // . K . L . M .
    // . . . . . . .
    float4 A = r_InputImage.Sample(r_Sampler, uv + texelSize * float2(-1.0f, -1.0f));
    float4 B = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 0.0f, -1.0f));
    float4 C = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 1.0f, -1.0f));
    float4 D = r_InputImage.Sample(r_Sampler, uv + texelSize * float2(-0.5f, -0.5f));
    float4 E = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 0.5f, -0.5f));
    float4 F = r_InputImage.Sample(r_Sampler, uv + texelSize * float2(-1.0f,  0.0f));
    float4 G = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 0.0f,  0.0f));
    float4 H = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 1.0f,  0.0f));
    float4 I = r_InputImage.Sample(r_Sampler, uv + texelSize * float2(-0.5f,  0.5f));
    float4 J = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 0.5f,  0.5f));
    float4 K = r_InputImage.Sample(r_Sampler, uv + texelSize * float2(-1.0f,  1.0f));
    float4 L = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 0.0f,  1.0f));
    float4 M = r_InputImage.Sample(r_Sampler, uv + texelSize * float2( 1.0f,  1.0f));
    
    float2 div = 0.25f * float2(0.5f, 0.125f);
    
    float4 color = (D + E + I + J) * div.x;
    color += (A + B + G + F) * div.y;
    color += (B + C + H + G) * div.y;
    color += (F + G + L + K) * div.y;
    color += (G + H + M + L) * div.y;
    
    // bright pass

    float luminance = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    
    if (luminance > u_Threshold)
        r_OutputImage[coord.xy] = color * u_Intensity;
    else
        r_OutputImage[coord.xy] = float4(0.0f, 0.0f, 0.0f, 0.0f);
}
