#pragma pack_matrix(column_major)

[[vk::binding(0, 0)]]
Texture2D<float4> r_LowMip;

[[vk::binding(1, 0)]]
SamplerState r_LowSampler;

[[vk::binding(2, 0)]]
Texture2D<float4> r_HighMip;

[[vk::binding(3, 0)]]
SamplerState r_HighSampler;

[[vk::binding(4, 0)]]
RWTexture2D<float4> r_DstImage;

[numthreads(16, 16, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
    int2 coord = int2(DTid.xy);
    float2 size;
    r_DstImage.GetDimensions(size.x, size.y);

    if (coord.x >= size.x || coord.y >= size.y)
        return;

    float2 uv = float2(coord) / size;

    float3 high = r_HighMip.Sample(r_HighSampler, uv).rgb;
    float3 low = r_LowMip.Sample(r_LowSampler, uv).rgb;

    float scatter = 0.5f;
    float3 color = lerp(high, low, scatter);
    r_DstImage[coord] = float4(color, 1.0f);
}
