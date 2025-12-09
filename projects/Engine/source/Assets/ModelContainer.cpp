#include "pch.h"
#include "include/Assets/ModelContainer.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Core/LogAssert.h"
#include "include/Graphics/ShaderStruct.h"
#include "include/Utility/TString.h"
#include "include/Utility/UniqueGenerate.h"

void Theatria::Assets::ModelContainer::CreateDefaultModels(Graphics::ResourceManager& rm)
{
    CreateCube();

    // VB,IBの再構築
    std::vector<VertexData> allVertices;
    std::vector<uint32_t> allIndices;
    size_t verSize = 0;
    size_t idxSize = 0;
    for (auto& model : m_Models)
    {
        for (auto& mesh : model.meshes)
        {
            verSize += mesh.vertices.size();
            idxSize += mesh.indices.size();
        }
    }
    allVertices.reserve(verSize);
    allIndices.reserve(idxSize);
    for (auto& model : m_Models)
    {
        for (auto& mesh : model.meshes)
        {
            // メッシュ情報設定
            mesh.meshInfo.baseVertex = static_cast<int32_t>(allVertices.size());
            mesh.meshInfo.indexOffset = static_cast<uint32_t>(allIndices.size());
            mesh.meshInfo.indexCount = static_cast<uint32_t>(mesh.indices.size());
            // 頂点、インデックス追加
            allVertices.insert(allVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            allIndices.insert(allIndices.end(), mesh.indices.begin(), mesh.indices.end());

            Graphics::GlobalBuffer<Graphics::ShaderStruct::SMeshInfo>& meshInfoBuf = rm.GetGlobalMeshInfoBuffer<Graphics::ShaderStruct::SMeshInfo>();
            mesh.modelIndex = meshInfoBuf.Allocate();
            std::span<Graphics::ShaderStruct::SMeshInfo> mappedData = meshInfoBuf.GetUploadBuffer().GetMappedData();
            mappedData[mesh.modelIndex] = mesh.meshInfo;
        }
    }
    // VB,IB再構築
    rm.RemakeIntegratedVBIB(allVertices, allIndices);
}

uint32_t Theatria::Assets::ModelContainer::Allocate(ModelData& model)
{
    std::wstring uName = model.name;

    if (m_ModelNameToIndex.contains(model.name))
    {
        uName = Theatria::Utility::GenerateUniqueName(model.name, m_ModelNameToIndex);
        model.name = uName;
    }

    uint32_t idx = 0;
    if (m_FreeList.empty())
    {
        idx = static_cast<uint32_t>(m_Models.size());
        m_Models.push_back(std::move(model));
        m_ModelNameToIndex[uName] = idx;
        return idx;
    }
    else
    {
        idx = m_FreeList.back();
        m_FreeList.pop_back();
        m_Models[idx] = std::move(model);
        m_ModelNameToIndex[uName] = idx;
        return idx;
    }
}

void Theatria::Assets::ModelContainer::CreateCube()
{
    // Cube
    std::wstring modelName = L"Cube";
    ModelData modelData;
    modelData.name = modelName;
    MeshData meshData;
    meshData.name = modelName;
    // 頂点数とインデックス数
    uint32_t vertices = 24;// 頂点数
    uint32_t indices = 36;// インデックス数
    // メモリ確保
    meshData.vertices.resize(vertices);
    meshData.indices.resize(indices);
    // 頂点データを設定
#pragma region
    // 右面
    meshData.vertices[0] = { {0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }; // 右上
    meshData.vertices[1] = { {0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }; // 左上
    meshData.vertices[2] = { {0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} }; // 右下
    meshData.vertices[3] = { {0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} }; // 左下

    // 左面
    meshData.vertices[4] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} }; // 左上
    meshData.vertices[5] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} }; // 右上
    meshData.vertices[6] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} }; // 左下
    meshData.vertices[7] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} }; // 右下

    // 前面
    meshData.vertices[8] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }; // 左上
    meshData.vertices[9] = { { 0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }; // 右上
    meshData.vertices[10] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }; // 左下
    meshData.vertices[11] = { { 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }; // 右下

    // 後面
    meshData.vertices[12] = { { 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 右上
    meshData.vertices[13] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 左上
    meshData.vertices[14] = { { 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}  }; // 右下
    meshData.vertices[15] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 左下

    // 上面
    meshData.vertices[16] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} }; // 左奥
    meshData.vertices[17] = { { 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} }; // 右奥
    meshData.vertices[18] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }; // 左前
    meshData.vertices[19] = { { 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }; // 右前

    // 下面
    meshData.vertices[20] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} }; // 左前
    meshData.vertices[21] = { { 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} }; // 右前
    meshData.vertices[22] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} }; // 左奥
    meshData.vertices[23] = { { 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} }; // 右奥

    // 右面インデックス
    meshData.indices[0] = 0; meshData.indices[1] = 2; meshData.indices[2] = 1;
    meshData.indices[3] = 2; meshData.indices[4] = 3; meshData.indices[5] = 1;

    // 左面インデックス
    meshData.indices[6] = 4; meshData.indices[7] = 6; meshData.indices[8] = 5;
    meshData.indices[9] = 6; meshData.indices[10] = 7; meshData.indices[11] = 5;

    // 前面インデックス
    meshData.indices[12] = 8; meshData.indices[13] = 10; meshData.indices[14] = 9;
    meshData.indices[15] = 10; meshData.indices[16] = 11; meshData.indices[17] = 9;

    // 後面インデックス
    meshData.indices[18] = 12; meshData.indices[19] = 14; meshData.indices[20] = 13;
    meshData.indices[21] = 14; meshData.indices[22] = 15; meshData.indices[23] = 13;

    // 上面インデックス
    meshData.indices[24] = 16; meshData.indices[25] = 18; meshData.indices[26] = 17;
    meshData.indices[27] = 18; meshData.indices[28] = 19; meshData.indices[29] = 17;

    // 下面インデックス
    meshData.indices[30] = 20; meshData.indices[31] = 22; meshData.indices[32] = 21;
    meshData.indices[33] = 22; meshData.indices[34] = 23; meshData.indices[35] = 21;
#pragma endregion
    // meshInfoに先頭オフセットを設定
    meshData.meshInfo.baseVertex = m_NextBaseVertexOffset;// 統合VB内の先頭オフセットを設定
    m_NextBaseVertexOffset += vertices;// 次のオフセットを進める
    meshData.meshInfo.indexOffset = m_NextBaseIndexOffset;// 統合IB内の先頭オフセットを設定
    m_NextBaseIndexOffset += indices;// 次のオフセットを進める
    meshData.meshInfo.indexCount = indices;// インデックス数を設定
    // メッシュ追加
    modelData.meshes.push_back(meshData);
    // コンテナに追加
    Allocate(modelData);
}
