#pragma once

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

struct ViewProjection
{
    float4x4 view;
    float4x4 projection;
};

struct MeshInfo
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

struct VSIn
{
    float4 position : POSITION;
};

struct VSOut
{
    float4 position : SV_POSITION;
};
