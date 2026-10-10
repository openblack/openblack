struct Vertex
{
    float3 position;
    float3 weight;
    // LandVertex stores these attributes as four packed bytes each.
    uint firstMaterialID;
    uint secondMaterialID;
    uint materialBlendCoefficient;
    uint lightLevel;
    float waterAlpha;
}

struct VOut
{
    float4 position : SV_Position;
    float2 uv;
    float2 footprintUv;
    float3 weight;
    nointerpolation uint3 firstMaterialID;
    nointerpolation uint3 secondMaterialID;
    float3 materialBlend;
    float lightLevel;
    float waterAlpha;
    float distance;
}

struct Camera
{
    float4x4 view;
    float4x4 viewProjection;
}

struct Block
{
    float4 positionAndSize;
    float4 islandExtent;
}

struct Terrain
{
    float4 skyAndBump;
}

[[vk::binding(0, 1)]] ConstantBuffer<Camera> camera;
[[vk::binding(1, 1)]] ConstantBuffer<Block> block;
[[vk::binding(0, 3)]] ConstantBuffer<Terrain> terrain;
[[vk::binding(0, 2)]] Sampler2DArray materials;
[[vk::binding(1, 2)]] Sampler2D bumpTexture;
[[vk::binding(2, 2)]] Sampler2D smallBumpTexture;
[[vk::binding(3, 2)]] Sampler2D footprintTexture;

uint3 unpackBytes(uint packed)
{
    return uint3(packed, packed >> 8, packed >> 16) & 255;
}

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    float4 worldPosition = float4(input.position.x + block.positionAndSize.x, input.position.y,
                                 input.position.z + block.positionAndSize.y, 1.0);
    VOut output;
    output.position = mul(camera.viewProjection, worldPosition);
    output.uv = input.position.zx / block.positionAndSize.wz;
    output.footprintUv = (worldPosition.xz - block.islandExtent.xy) /
                         (block.islandExtent.zw - block.islandExtent.xy);
    output.footprintUv.y = 1.0 - output.footprintUv.y;
    output.weight = input.weight;
    output.firstMaterialID = unpackBytes(input.firstMaterialID);
    output.secondMaterialID = unpackBytes(input.secondMaterialID);
    output.materialBlend = float3(unpackBytes(input.materialBlendCoefficient)) / 255.0;
    output.lightLevel = float(input.lightLevel & 255) / 255.0;
    output.waterAlpha = input.waterAlpha;
    output.distance = mul(camera.view, worldPosition).z;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    if (input.waterAlpha == 0.0)
        discard;

    float3 color = float3(0.0);
    for (int i = 0; i < 3; ++i)
    {
        float3 first = materials.Sample(float3(input.uv, input.firstMaterialID[i])).rgb;
        float3 second = materials.Sample(float3(input.uv, input.secondMaterialID[i])).rgb;
        color += lerp(first, second, input.materialBlend[i]) * input.weight[i];
    }
    color *= lerp(1.0, bumpTexture.Sample(input.uv).r * 2.0, terrain.skyAndBump.y);
    if (input.distance < 200.0)
    {
        float strength = (1.0 - input.distance / 200.0) * terrain.skyAndBump.z;
        color *= 1.0 - smallBumpTexture.Sample(input.uv * 10.0).r * strength;
    }
    float4 footprint = footprintTexture.Sample(input.footprintUv);
    color = lerp(color, footprint.rgb, footprint.a);
    color *= lerp(0.25, clamp(input.lightLevel * 2.0, 0.5, 1.0), terrain.skyAndBump.x / 2.0);
    return float4(color, input.waterAlpha);
}
