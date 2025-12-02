#pragma once

struct VertexInfluence {
    float4 weight;
    int4 index;
};
struct SkinningInformation {
    uint numVertices;
    uint boneCount;
    uint isSkinned;
};
struct Well {
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};