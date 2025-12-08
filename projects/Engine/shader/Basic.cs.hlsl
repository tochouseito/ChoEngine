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

struct ModelData
{
    uint indexOffset;
    uint indexCount;
    int baseVertex;
    uint pad;
};

struct IndirectCommand
{
    uint ObjectId;
    uint _pad0;
    uint _pad1;
    uint _pad2;

    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};

// SRV
StructuredBuffer<Object> g_Objects : register(t0);
StructuredBuffer<Transform> g_Transforms : register(t1);
StructuredBuffer<ModelData> g_Models : register(t2);

// UAV
RWStructuredBuffer<IndirectCommand> g_IndirectCommands : register(u0);
RWByteAddressBuffer g_CommandCount : register(u1);

cbuffer DispatchParam : register(b0)
{
    uint g_ObjectCount;
};

[numthreads(64, 1, 1)]
void CSBuildIndirect(uint3 dtid : SV_DispatchThreadID)
{
    uint objIndex = dtid.x;
    if (objIndex >= g_ObjectCount)
        return;

    Object obj = g_Objects[objIndex];
    if (obj.visible == 0)
        return;

    ModelData model = g_Models[obj.modelId];

    // g_CommandCount[0] を原子的にインクリメント
    uint dstIndex;
    g_CommandCount.InterlockedAdd(0, 1, dstIndex);

    IndirectCommand cmd;
    cmd.ObjectId = objIndex;
    cmd._pad0 = cmd._pad1 = cmd._pad2 = 0;

    cmd.IndexCountPerInstance = model.indexCount;
    cmd.InstanceCount = 1;
    cmd.StartIndexLocation = model.indexOffset;
    cmd.BaseVertexLocation = model.baseVertex;
    cmd.StartInstanceLocation = 0;

    g_IndirectCommands[dstIndex] = cmd;
}
