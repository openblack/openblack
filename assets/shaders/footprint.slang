struct Vertex
{
    float2 position;
    float2 uv;
}

struct FootprintData
{
    float4x4 viewProjection;
    uint4 offsets;
}

struct VOut
{
    float4 position : SV_Position;
    float2 uv;
}

[[vk::binding(0, 0)]] StructuredBuffer<float4x4> matrices;
[[vk::binding(0, 1)]] ConstantBuffer<FootprintData> footprint;
[[vk::binding(0, 2)]] Sampler2D diffuseTexture;

[shader("vertex")]
VOut vertexMain(Vertex input, uint instanceID : SV_InstanceID)
{
    float4 worldPosition = mul(matrices[footprint.offsets.x + instanceID],
                               float4(input.position.x, 0, input.position.y, 1));
    VOut output;
    output.position = mul(footprint.viewProjection, worldPosition);
    // Height does not affect the orthographic footprint map, which has no depth target.
    output.position.z = 0;
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    return diffuseTexture.Sample(input.uv);
}
