#include "Basic.hlsli"

// BindResources

// ViewProjection
ConstantBuffer<ViewProjection> gVP : register(b0, space0);
// Transform
StructuredBuffer<Transform> gTransforms : register(t0, space0);
// 
