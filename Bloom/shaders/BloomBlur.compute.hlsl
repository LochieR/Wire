#pragma pack_matrix(column_major)

[[vk::push_constant]]
cbuffer PushConstants
{
    int2 u_FullSize;
    int u_Horizontal;
};

[[vk::binding(0, 0)]]
Texture2D<float4> r_SrcImage;

[[vk::binding(1, 0)]]
SamplerState r_Sampler;

[[vk::binding(2, 0)]]
RWTexture2D<float4> r_DstImage;

[numthreads(16, 16, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
    int2 coord = int2(DTid.xy);

    if (coord.x >= u_FullSize.x || coord.y >= u_FullSize.y)
        return;

    float2 texelSize = 1.0f / u_FullSize;
    float2 uv = (float2(coord) + 0.5) * texelSize;

    const float weights[5] = { 0.204164, 0.304005, 0.093913, 0.010381, 0.001414 };
    float2 offsets[5];
    for (int i = 0; i < 5; i++)
    {
        float offset = float(i);
        offsets[i] = u_Horizontal != 0 ? float2(offset, 0.0) * texelSize : float2(0.0, offset) * texelSize;
    }

    float3 color = r_SrcImage.Sample(r_Sampler, uv).rgb * weights[0];
    for (int i = 1; i < 5; i++)
    {
        color += r_SrcImage.Sample(r_Sampler, uv + offsets[i]).rgb * weights[i];
        color += r_SrcImage.Sample(r_Sampler, uv - offsets[i]).rgb * weights[i];
    }

    r_DstImage[coord] = float4(color, 1.0f);
}
