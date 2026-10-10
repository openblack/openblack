struct Vertex
{
    float4 position;
    float4 color;
}

struct LineData
{
    float4x4 viewProjection;
    uint4 offsets;
}

struct VOut
{
    float4 position : SV_Position;
    float4 color;
}

[[vk::binding(0, 0)]] StructuredBuffer<float4x4> matrices;
[[vk::binding(0, 1)]] ConstantBuffer<LineData> line;

[shader("vertex")]
VOut vertexMain(Vertex input, uint instanceID : SV_InstanceID)
{
    VOut output;
    output.position = mul(line.viewProjection, mul(matrices[line.offsets.x + instanceID], input.position));
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    return input.color;
}
