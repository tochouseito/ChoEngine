#pragma once
// === C++ Standard Library ===
#include <cstdint>

// === Theatria Engine Include ===

// === Theatria Math Include ===
#include <ChoMath/include/Vector2.h>
#include <ChoMath/include/Vector3.h>
#include <ChoMath/include/Vector4.h>
#include <ChoMath/include/Matrix4.h>

namespace Theatria::Graphics::ShaderStruct
{
    using namespace Theatria::Math;
    struct SObject
    {
        uint32_t id;
        uint32_t visible;
        uint32_t modelId;
        uint32_t transformId;
    };

    struct STransform
    {
        float4x4 worldMatrix;
    };

    struct SViewProjection
    {
        float4x4 view;
        float4x4 projection;
    };

    struct SModelInfo
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        int32_t baseVertex;
        uint32_t pad;
    };
}
