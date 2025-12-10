struct Object
{
    uint id;
    uint visible;
    uint meshId;
    uint transformId;
};

struct Transform
{
    float4x4 worldMatrix;
};

struct MeshData
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

    uint _pad3;
    uint _pad4;
    uint _pad5;
};

// SRV
StructuredBuffer<Object> g_Objects : register(t0);
StructuredBuffer<Transform> g_Transforms : register(t1);
StructuredBuffer<MeshData> g_Meshes : register(t2);

// UAV
RWStructuredBuffer<IndirectCommand> g_IndirectCommands : register(u0);
RWByteAddressBuffer g_CommandCount : register(u1);

cbuffer DispatchParam : register(b0)
{
    uint g_ObjectCount;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint objIndex = dtid.x;
    if (objIndex >= g_ObjectCount)
        return;

    Object obj = g_Objects[objIndex];
    if (obj.visible == 0)
        return;

    MeshData model = g_Meshes[obj.meshId];

    // g_CommandCount[0] を原子的にインクリメント
    uint dstIndex = 0;
    g_CommandCount.InterlockedAdd(0, 1, dstIndex);

    g_IndirectCommands[dstIndex].ObjectId = objIndex;
    g_IndirectCommands[dstIndex]._pad0 = 0;
    g_IndirectCommands[dstIndex]._pad1 = 0;
    g_IndirectCommands[dstIndex]._pad2 = 0;

    g_IndirectCommands[dstIndex].IndexCountPerInstance = model.indexCount;
    g_IndirectCommands[dstIndex].InstanceCount = 1;
    g_IndirectCommands[dstIndex].StartIndexLocation = model.indexOffset;
    g_IndirectCommands[dstIndex].BaseVertexLocation = model.baseVertex;
    g_IndirectCommands[dstIndex].StartInstanceLocation = 0;

    g_IndirectCommands[dstIndex]._pad3 = 0;
    g_IndirectCommands[dstIndex]._pad4 = 0;
    g_IndirectCommands[dstIndex]._pad5 = 0;
}
