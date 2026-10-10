struct Vertex
{
    float2 position;
}

struct SpriteData
{
    float4x4 viewProjection;
    float4x4 model;
    float4x4 inverseView;
    float4 sampleRect;
}

struct VOut
{
    float4 position : SV_Position;
    float2 uv;
}

[[vk::binding(0, 1)]] ConstantBuffer<SpriteData> sprite;
[[vk::binding(0, 2)]] Sampler2D diffuseTexture;
[[vk::binding(0, 3)]] ConstantBuffer<float4> tint;

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    float2 uv = float2(input.position.x * 0.5 + 0.5, 0.5 - input.position.y * 0.5);
    VOut output;
    float4 local = float4(input.position, 0.0, 0.0);
    float3 translation = mul(sprite.model, float4(0, 0, 0, 1)).xyz;
    float3 scaled = mul(sprite.model, local).xyz;
    float3 billboard = mul(sprite.inverseView, float4(scaled, 0)).xyz;
    output.position = mul(sprite.viewProjection, float4(translation + billboard, 1));
    output.uv = uv * sprite.sampleRect.xy + sprite.sampleRect.zw;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    return diffuseTexture.Sample(input.uv).rrrr * tint;
}
