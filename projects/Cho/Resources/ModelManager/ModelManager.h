#pragma once
#include "ChoMath.h"
#include "Core/Utility/FVector.h"
#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"
#include "Core/Utility/Color.h"
#include "Core/Utility/AnimationStruct.h"
#include "Graphics/PipelineManager/PipelineManager.h"
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <list>
#include <map>
// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/version.h>
// 頂点データ構造体
struct VertexData
{
    float4 position;
    float2 texCoord;
	float3 normal;
	Color color = { 1.0f,1.0f,1.0f,1.0f };
	uint32_t vertexID = 0;
};
struct Node
{
	NodeTransform transform;
	float4x4 localMatrix;
	std::string name;
	std::string parentName;
	std::vector<Node> children;
};
struct MaterialData
{
	Color color = { 1.0f,1.0f,1.0f,1.0f };
	std::string textureName = "";
};

// アニメーションデータ構造体
struct AnimationData
{
	std::string name;
	float duration = 0.0f;// アニメーション全体の尺（単位は秒）
	uint32_t allFrame = 0;// アニメーション全体のフレーム数
	std::map<std::string, NodeAnimation> nodeAnimations;
};
struct MeshData
{
	std::wstring name;
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	std::vector<MaterialData> materials;
	std::map<std::string, JointWeightData>skinClusterData;
	std::optional<uint32_t> vertexBufferIndex = std::nullopt;
	std::optional<uint32_t> indexBufferIndex = std::nullopt;
	SkinCluster skinCluster;

	// スキニング情報のバッファインデックス
	std::optional<uint32_t> influenceBufferIndex = std::nullopt; // スキニング情報のバッファインデックス
	std::optional<uint32_t> skinInfoBufferIndex = std::nullopt;	// スキニング情報バッファーインデックス
};
struct ModelData
{
	std::wstring name;
	std::vector<MeshData> meshes;
	Node rootNode;
	// このモデルを使用しているTransformのインデックス
	std::list<uint32_t> useTransformList;
	// このモデルを使用しているTFリストのバッファインデックス
	std::optional<uint32_t> useTransformBufferIndex = std::nullopt;
	bool isBone = false;
	std::vector<AnimationData> animations;
	Skeleton skeleton;
	// ボーン行列統合バッファインデックス
	std::optional<uint32_t> boneMatrixBufferIndex = std::nullopt;
	uint32_t nextBoneOffsetIndex = 0; // 次のボーンオフセットインデックス
	std::vector<uint32_t> removedBoneOffsetIndices; // 削除されたボーンオフセットインデックスのリスト

	//ArgsBuffer argsBuffer;// ArgsBuffer

	uint32_t AllocateBoneOffsetIdx()
	{
		if (removedBoneOffsetIndices.empty())
		{
			uint32_t result = nextBoneOffsetIndex;
			nextBoneOffsetIndex++;
			return result;
		}
		else
		{
			uint32_t idx = removedBoneOffsetIndices.back();
			removedBoneOffsetIndices.pop_back();
			return idx;
		}
	}
	void RemoveBoneOffsetIdx(const uint32_t& idx)
	{
		removedBoneOffsetIndices.push_back(idx);
	}
};

struct IntegrationMesh
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	uint32_t vertexBufferIndex = 0;
	uint32_t indexBufferIndex = 0;
	std::unordered_map<std::wstring, std::pair<uint32_t, uint32_t>> offsetMap;
};

class ResourceManager;
class ModelManager
{
public:
	ModelManager(ResourceManager* resourceManager)
		:m_pResourceManager(resourceManager)
	{
		// Assimpのオプションチェック
		CheckAssimpOption();
		CreateDefaultMesh();
	}
	~ModelManager()
	{

	}
	// モデルファイルを読み込む
	bool LoadModelFile(const std::filesystem::path& filePath);

	// 名前コンテナを取得する
	std::unordered_map<std::wstring, uint32_t>& GetModelNameContainer() { return m_ModelNameContainer; }
	// モデルデータコンテナを取得する
	FVector<ModelData>& GetModelDataContainer() { return m_Models; }
	// 名前で検索してインデックスを取得する
	std::optional<uint32_t> GetModelDataIndex(const std::wstring& name);
	// モデルデータを取得する
	ModelData* GetModelData(const uint32_t& index) { return &m_Models[index]; }
	ModelData* GetModelData(const std::wstring& name)
	{
		if (m_ModelNameContainer.contains(name))
		{
			return &m_Models[m_ModelNameContainer[name]];
		}
		return nullptr;
	}
	// モデルのUseListに登録する
	void RegisterModelUseList(const std::variant<uint32_t,std::wstring>& key, const uint32_t& transformMapID);
	// モデルのUseListから削除する
	void RemoveModelUseList(const std::variant<uint32_t, std::wstring>& key, const uint32_t& transformMapID);

	// Effect用のメッシュデータを取得する
	MeshData& GetEffectRingMeshData() { return m_EffectRingMeshData; }
private:
	// Assimpのオプションチェック
	void CheckAssimpOption();
	// デフォルトメッシュの生成
	void CreateDefaultMesh();
	// Cubeの生成
	void CreateCube();
	// Sphereの生成
	void CreateSphere();
	// Planeの生成
	void CreatePlane();
	// Ringの生成
	void CreateRing();
	// Cylinderの生成
	void CreateCylinder();
	// Skyboxの生成
	void CreateSkybox();
	// EffectRingの生成
	void CreateEffectRing();

	// 
	Node ReadNode(aiNode* node, const std::string& parentName);
	// ジョイントの生成
	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);
	// モデルコンテナの要素数を取得する
	//uint32_t GetModelDataSize() { return static_cast<uint32_t>(m_Models.size()); }
	// ModelDataの追加
	void AddModelData(ModelData& modelData);

	ResourceManager* m_pResourceManager = nullptr;
	// モデルデータコンテナ
	FVector<ModelData> m_Models;
	// モデルのキーを名前で管理するコンテナ
	std::unordered_map<std::wstring, uint32_t> m_ModelNameContainer;

	// Effect用メッシュデータ
	MeshData m_EffectRingMeshData;

	// モデルの使用可能なTransformの数のオフセット
	static const uint32_t kUseTransformOffset = 1024;

	std::vector<IntegrationMesh> m_IntegrationMeshes; // 統合メッシュデータ
};

