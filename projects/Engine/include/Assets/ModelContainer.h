#pragma once

// C++ Standard Library
#include <vector>
#include <string>
#include <unordered_map>
// Theatria Engine Includes
#include "include/Graphics/ShaderStruct.h"
// Theatria Math Library
#include <ChoMath/include/Vector2.h>
#include <ChoMath/include/Vector3.h>
#include <ChoMath/include/Vector4.h>
#include <ChoMath/include/Matrix4.h>

namespace Theatria
{
    namespace Graphics
    {
        class RenderDevice;
        class Renderer;
        class ResourceManager;
    }
    namespace Assets
    {
        struct VertexData
        {
            Theatria::Math::float4 position;
            Theatria::Math::float2 texCoord;
            Theatria::Math::float3 normal;
        };

        struct MeshData
        {
            std::wstring name = L"";
            std::vector<VertexData> vertices;
            std::vector<uint32_t> indices;

            Graphics::ShaderStruct::SMeshInfo meshInfo = {};
            uint32_t modelIndex = UINT32_MAX;
        };

        struct ModelData
        {
            std::wstring name = L"";
            std::vector<MeshData> meshes;
        };

        /// @brief メッシュコンテナ
        class ModelContainer final
        {
        public:
            ModelContainer() = default;
            ~ModelContainer() = default;

            /// @brief デフォルトモデルの生成
            void CreateDefaultModels(Graphics::ResourceManager& rm);

            /// @brief モデルの追加
        private:
            uint32_t Allocate(ModelData& model);

            void CreateCube();

            std::vector<ModelData> m_Models;
            std::vector<uint32_t> m_FreeList;
            std::unordered_map<std::wstring, uint32_t> m_ModelNameToIndex;
            uint32_t m_NextBaseVertexOffset = 0;
            uint32_t m_NextBaseIndexOffset = 0;
        };
    }
}

