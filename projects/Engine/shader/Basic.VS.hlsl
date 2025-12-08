struct Object
{
    uint id;
    uint visible;
    uint modelId;
    uint transformId;
};

struct Transform
{
    float4x4 worldMatrix;
};

struct ViewProjection
{
    float4x4 view;
    float4x4 projection;
};

cbuffer ViewProjectionCB : register(b0)
{
    ViewProjection g_VP;
};

// RootConstant (1OD Command から来る)
cbuffer ObjectIdCB : register(b1)
{
    uint g_ObjectId;
};

StructuredBuffer<Object> g_Objects : register(t0);
StructuredBuffer<Transform> g_Transforms : register(t1);
// ModelData は VS では不要なので参照しなくていい
// StructuredBuffer<ModelData> g_Models : register(t2);

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;

    Object obj = g_Objects[g_ObjectId];
    Transform tr = g_Transforms[obj.transformId];

    float4 localPos = float4(input.pos, 1.0f);
    float4 worldPos = mul(localPos, tr.worldMatrix);
    float4 viewPos = mul(worldPos, g_VP.view);
    float4 projPos = mul(viewPos, g_VP.projection);

    o.pos = projPos;
    o.normal = mul(float4(input.normal, 0.0f), tr.worldMatrix).xyz;
    o.uv = input.uv;

    return o;
}
