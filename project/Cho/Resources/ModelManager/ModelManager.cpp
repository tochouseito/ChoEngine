#include "pch.h"
#include "ModelManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "SDK/DirectX/DirectX12/VertexBuffer/VertexBuffer.h"
#include "SDK/DirectX/DirectX12/IndexBuffer/IndexBuffer.h"
#include "ChoMath.h"
#include "Core/ChoLog/ChoLog.h"
#include "Core/Utility/GenerateUnique.h"
#include <filesystem>
using namespace theatria;

bool ModelManager::LoadModelFile(const std::filesystem::path& filePath)
{
	// 対応フォーマット:.fbx, .obj, .gltf, .blend, .mmd
	// blend, mmdは未対応
	Log::Write(LogLevel::Info, "LoadModelFile");
	// 変数の宣言
	std::string line;// ファイルから読んだ1行を格納するもの
	Assimp::Importer importer;
	std::wstring filePathString = filePath.wstring();
	ModelData modelData;

	// ここからファイルを開く
	Log::Write(LogLevel::Info, "Start ReadFile");

	// 線、点を捨てる
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

	const aiScene* scene = importer.ReadFile(
		ConvertString(filePathString).c_str(),
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs |
		aiProcess_Triangulate|				// 三角形化
		//aiProcess_ValidateDataStructure|	// 読み込んだシーンの構造が内部的に正しいかチェックし、異常があればログに出す
		aiProcess_JoinIdenticalVertices|		// 「位置・法線・UV」などすべての属性が同一の頂点をマージして1つにまとめる
		aiProcess_SortByPType|					// プリミティブタイプ（点・線・三角形など）を分類します
		aiProcess_ImproveCacheLocality		// インデックスバッファをGPUキャッシュに優しい順番に並び替える
		//aiProcess_RemoveRedundantMaterials|	// 他と同じ内容のマテリアルを1つに統合します
		//aiProcess_FindDegenerates				// 退化ポリゴン（面積ゼロの三角形、重なった点）を検出・削除します ←indexが壊れるのでOFF
		//aiProcess_FindInvalidData				// NaNやInfなどの無効な頂点データや法線を検出・修正します
		);
	if (!scene)
	{
        Log::Write(LogLevel::Assert, L"Assimp ReadFile Error: file name = " + filePathString);
	}
	Log::Write(LogLevel::Info, "End ReadFile");
	std::string err = importer.GetErrorString();
	err = "ErrorString:" + err;
	Log::Write(LogLevel::Info, err);
	if (!scene->HasMeshes())
	{
		Log::Write(LogLevel::Assert, "No Meshes");// メッシュがないのは対応しない
	}
	// SceneのRootNodeを読んでシーン全体の階層構造を作り上げる
	modelData.rootNode = ReadNode(scene->mRootNode,"");
	// 名前取得
	modelData.name = filePath.stem().wstring();
	modelData.name = GenerateUniqueName(modelData.name, m_ModelNameContainer);

	// メッシュ解析
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		MeshData meshData;
		aiMesh* mesh = scene->mMeshes[meshIndex];
		meshData.name = ConvertString(mesh->mName.C_Str());
		// 頂点数とインデックス数を取得
		uint32_t vertexCount = mesh->mNumVertices;// 頂点数
		uint32_t indexCount = mesh->mNumFaces * 3;// インデックス数(三角形化されているので3倍)
		// メモリ確保
		meshData.vertices.resize(vertexCount);
		meshData.indices.resize(indexCount);
		// 頂点情報のコピー
		if (!mesh->HasNormals())
		{
			// 法線がないメッシュは非対応
			Log::Write(LogLevel::Assert, "Mesh has no normals. This is not supported.");
		}
		for (uint32_t vi = 0;vi < vertexCount; ++vi)
		{
			VertexData& dst = meshData.vertices[vi];
			const aiVector3D& p = mesh->mVertices[vi];
			const aiVector3D& n = mesh->mNormals[vi];
			dst.position = { -p.x, p.y, p.z, 1.0f };// X軸反転
			dst.normal = { -n.x, n.y, n.z };// X軸反転
			if (mesh->HasTextureCoords(0))
			{
				const aiVector3D& t = mesh->mTextureCoords[0][vi];
				dst.texCoord = { t.x, t.y };
			} else
			{
				dst.texCoord = { 0.0f, 0.0f };// UV座標がない場合はダミー
			}
			dst.color = { 1.0f, 1.0f, 1.0f, 1.0f };// 色は白
			dst.vertexID = vi;// 頂点ID
		}
		// インデックス情報のコピー
		uint32_t idx = 0;
		for (uint32_t fi = 0;fi < mesh->mNumFaces;++fi)
		{
			const aiFace& face = mesh->mFaces[fi];
			// aiProcess_Triangulate を使っているので常に face.mNumIndices == 3
			for (uint32_t e = 0;e < 3;++e)
			{
				if (face.mIndices[e] >= vertexCount)
				{
					Log::Write(LogLevel::Assert, "MeshData Index out of range: " + std::to_string(face.mIndices[e]));
				}
				meshData.indices[idx++] = face.mIndices[e];
			}
		}
		// チェック
		Log::Write(LogLevel::Assert, "MeshData IndexCount Check", idx == indexCount);

		// マテリアル解析
		if (scene->HasMaterials())
		{
			uint32_t materialIndex = mesh->mMaterialIndex;
			Log::Write(LogLevel::Assert, "MeshData MaterialIndex Check", materialIndex < scene->mNumMaterials);
			aiMaterial* aiMat = scene->mMaterials[materialIndex];
			MaterialData materialData;

			// 拡散色(diffuse Color)の取得
			aiColor4D diffuseColor;
			if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor))
			{
				materialData.color = { diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a };
			} else
			{
				// デフォルトの色を設定
				materialData.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
				Log::Write(LogLevel::Warn, "Material has no diffuse color, using default white.");
			}

			// 拡散テクスチャ(diffuse Texture)の取得
			if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString path;
				if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &path))
				{
					std::filesystem::path texturePath{ path.C_Str() };
					// ディレクトリパス、拡張子を除去
					materialData.textureName = texturePath.stem().string();
				}
			}
			// MaterialDataを追加
			meshData.materials.push_back(materialData);
		}

		// アニメーション解析
		if (scene->HasAnimations())
		{
			// アニメーションを読み込む
			for (uint32_t animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
			{
				AnimationData animation; // 今回作成するアニメーションデータ
				animation.name = scene->mAnimations[animationIndex]->mName.C_Str(); // アニメーション名を取得
				aiAnimation* animationAssimp = scene->mAnimations[animationIndex];
				animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // アニメーションの時間を秒単位に変換

				// チャネル（ノードのアニメーション情報）の解析
				for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex)
				{
					aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];

					// ノードのアニメーション情報を取得
					NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

					// 平行移動（Translate）のキーフレームを解析
					for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex)
					{
						aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
						Keyframefloat3 keyframe;
						keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 時間を秒単位に変換
						keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // 右手座標系を左手座標系に変換
						nodeAnimation.translate.keyframes.push_back(keyframe);
					}

					// 回転（Rotate）のキーフレームを解析
					for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex)
					{
						aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
						KeyframeQuaternion keyframe;
						keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 時間を秒単位に変換
						keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w }; // 右手座標系を左手座標系に変換（y と z を反転）
						nodeAnimation.rotate.keyframes.push_back(keyframe);
					}

					// スケール（Scale）のキーフレームを解析
					for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex)
					{
						aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
						KeyframeScale keyframe;
						keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 時間を秒単位に変換
						keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // スケールはそのまま変換なし
						nodeAnimation.scale.keyframes.push_back(keyframe);
					}
				}

				// アニメーション解析完了後、ModelData に追加
				modelData.animations.push_back(animation);
			}
		}
		// ボーン解析
		if (mesh->mNumBones)
		{
			modelData.isBone = true;
			// ボーンの情報を取得
			Skeleton skeleton;
			skeleton.root = CreateJoint(modelData.rootNode, {}, skeleton.joints);
			// 名前とindexのマッピングを行いアクセスしやすくする
			for (const Joint& joint : skeleton.joints)
			{
				skeleton.jointMap.emplace(joint.name, joint.index);
			}
			modelData.skeleton = skeleton;
			// スキンクラスタの情報を取得
			SkinCluster skinCluster;
			skinCluster.paletteData.data.resize(modelData.skeleton.joints.size());
			skinCluster.influenceData.data.resize(vertexCount);
			std::memset(skinCluster.influenceData.data.data(), 0, sizeof(ConstBufferDataVertexInfluence) * vertexCount);// Influenceの初期化
			/*InverseBindPoseMatrixの保存領域を作成*/
			skinCluster.inverseBindPoseMatrices.resize(modelData.skeleton.joints.size());
			std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), []() { return float4x4::Identity(); });
			meshData.skinCluster = skinCluster;

			for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
			{
				aiBone* bone = mesh->mBones[boneIndex];// AssimpではJointをBoneと呼んでいる
				std::string jointName = bone->mName.C_Str();
				JointWeightData& jointWeightData = meshData.skinClusterData[jointName];

				aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();// BindPoseMatrixに戻す
				aiVector3D scale, translate;
				aiQuaternion rotate;
				bindPoseMatrixAssimp.Decompose(scale, rotate, translate);// 成分を抽出
				/*左手系のBindPoseMatrixを作る*/
				float4x4 bindPoseMatrix = Theatria::Math::MakeAffineMatrix(
					{ scale.x,scale.y,scale.z }, { rotate.x,-rotate.y,-rotate.z,rotate.w }, { -translate.x,translate.y,translate.z });
				/*InverseBindPoseMatrixにする*/
				jointWeightData.inverseBindPoseMatrix = float4x4::Inverse(bindPoseMatrix);

				for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
				{
					jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
				}
			}
			// Influence作成
			/*ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める*/
			for (const auto& jointWeight : meshData.skinClusterData)
			{
				// ModelのSkinClusterの情報を解析
				auto it = modelData.skeleton.jointMap.find(jointWeight.first);// jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
				if (it == modelData.skeleton.jointMap.end())
				{
					// そんな名前のjointは存在しない、なので次に回す
					continue;
				}
				/*(*it).secondにはJointのIndexが入っているので、該当のindexのInverseBindPoseMatrixを代入*/
				meshData.skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
				for (const auto& vertexWeight : jointWeight.second.vertexWeights)
				{
					auto& currentInfluence = meshData.skinCluster.influenceData.data[vertexWeight.vertexIndex];// 該当のvertexIndexのinfluence情報を参照しておく
					for (uint32_t index = 0; index < kNumMaxInfluence; ++index)
					{
						// 空いてるところに入れる
						if (currentInfluence.weights[index] == 0.0f)
						{// weight==0が空いてる状態なので、その場所にweightとjointのIndexを代入
							currentInfluence.weights[index] = vertexWeight.weight;
							currentInfluence.jointIndices[index] = (*it).second;
							break;
						}
					}
				}
			}
		}
		// メッシュデータをモデルデータに追加
		modelData.meshes.push_back(meshData);
	}
	// modelDataを追加
	AddModelData(modelData);
	return true;
}

// 名前で検索してインデックスを取得する
std::optional<uint32_t> ModelManager::GetModelDataIndex(const std::wstring& name)
{
	if (m_ModelNameContainer.contains(name))
	{
		return m_ModelNameContainer[name];
	} else
	{
		std::string modelName = ConvertString(name);
		std::string msg = "Model name not found: " + modelName;
		Log::Write(LogLevel::Assert, msg);
	}
	return std::nullopt;
}

void ModelManager::RegisterModelUseList(const std::variant<uint32_t, std::wstring>& key, const uint32_t& transformMapID)
{
	// コンテナのキー
	uint32_t keyIndex = 0;
	if (std::holds_alternative<uint32_t>(key))
	{
		keyIndex = std::get<uint32_t>(key);
	} else if (std::holds_alternative<std::wstring>(key))
	{
		std::wstring name = std::get<std::wstring>(key);
		if (!m_ModelNameContainer.contains(name))
		{
			Log::Write(LogLevel::Assert, "Model name not found: " + ConvertString(name));
			return;
		}
		keyIndex = m_ModelNameContainer[name];
	}
	// 既にUseListに登録されている場合は登録しない
	if (!m_Models[keyIndex].useTransformList.empty())
	{
		// 指定されたモデルに登録されているか確認
		for (auto& index : m_Models[keyIndex].useTransformList)
		{
			if (index == transformMapID)
			{
				return;
			}
		}
	}
	// ほかのモデルに登録されていたら、削除
	for (auto& model : m_Models)
	{
		if (model.useTransformList.size() > 0)
		{
			for (auto& index : model.useTransformList)
			{
				if (index == transformMapID)
				{
					model.useTransformList.remove(index);
					// UseListのバッファ更新
					StructuredBuffer<uint32_t>* buffer = dynamic_cast<StructuredBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(model.useTransformBufferIndex));
					if (buffer)
					{
						// UseListの全てをバッファに転送
						uint32_t i = 0;
						for (uint32_t& useIndex : model.useTransformList)
						{
							buffer->UpdateData(useIndex, i);
							i++;
						}
					}
					else
					{
						Log::Write(LogLevel::Assert, "Buffer is nullptr");
					}
					break;
				}
			}
		}
	}
	// どのモデルのUseListにない場合は、追加
	m_Models[keyIndex].useTransformList.push_back(transformMapID);
	// UseListのバッファ更新
	StructuredBuffer<uint32_t>* buffer = dynamic_cast<StructuredBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(m_Models[keyIndex].useTransformBufferIndex));
	if (buffer)
	{
		// UseListの全てをバッファに転送
		uint32_t i = 0;
		for (uint32_t& useIndex : m_Models[keyIndex].useTransformList)
		{
			buffer->UpdateData(useIndex, i);
			i++;
		}
	} else
	{
		Log::Write(LogLevel::Assert, "Buffer is nullptr");
	}
}

void ModelManager::RemoveModelUseList(const std::variant<uint32_t, std::wstring>& key, const uint32_t& transformMapID)
{
	// コンテナのキー
	uint32_t keyIndex = 0;
	if (std::holds_alternative<uint32_t>(key))
	{
		keyIndex = std::get<uint32_t>(key);
	} else if (std::holds_alternative<std::wstring>(key))
	{
		std::wstring name = std::get<std::wstring>(key);
		if (!m_ModelNameContainer.contains(name))
		{
			Log::Write(LogLevel::Assert, "Model name not found: " + ConvertString(name));
			return;
		}
		keyIndex = m_ModelNameContainer[name];
	}
	// 既にUseListに登録されている場合は登録しない
	if (!m_Models[keyIndex].useTransformList.empty())
	{
		// 指定されたモデルに登録されているか確認、削除する
		for (auto& index : m_Models[keyIndex].useTransformList)
		{
			if (index == transformMapID)
			{
				// 見つかったら削除
				m_Models[keyIndex].useTransformList.remove(transformMapID);
				// UseListのバッファ更新
				StructuredBuffer<uint32_t>* buffer = dynamic_cast<StructuredBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(m_Models[keyIndex].useTransformBufferIndex));
				if (buffer)
				{
					// UseListの全てをバッファに転送
					uint32_t i = 0;
					for (uint32_t& useIndex : m_Models[keyIndex].useTransformList)
					{
						buffer->UpdateData(useIndex, i);
						i++;
					}
				}
				else
				{
					Log::Write(LogLevel::Assert, "Buffer is nullptr");
				}
				break;
			}
		}
		// ほかのモデルに登録されていたら、削除
		for (auto& model : m_Models)
		{
			if (model.useTransformList.size() > 0)
			{
				for (auto& index : model.useTransformList)
				{
					if (index == transformMapID)
					{
						model.useTransformList.remove(transformMapID);
						// UseListのバッファ更新
						StructuredBuffer<uint32_t>* buffer = dynamic_cast<StructuredBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(model.useTransformBufferIndex));
						if (buffer)
						{
							// UseListの全てをバッファに転送
							uint32_t i = 0;
							for (uint32_t& useIndex : model.useTransformList)
							{
								buffer->UpdateData(useIndex, i);
								i++;
							}
						}
						else
						{
							Log::Write(LogLevel::Assert, "Buffer is nullptr");
						}
						break;
					}
				}
			}
		}
	}
	// どのモデルのUseListにない場合は終わり
	// UseListのバッファ更新


	StructuredBuffer<uint32_t>* buffer = dynamic_cast<StructuredBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(m_Models[keyIndex].useTransformBufferIndex));
	if (buffer)
	{
		// UseListの全てをバッファに転送
		uint32_t i = 0;
		for (uint32_t& useIndex : m_Models[keyIndex].useTransformList)
		{
			buffer->UpdateData(useIndex, i);
			i++;
		}
	} else
	{
		Log::Write(LogLevel::Assert, "Buffer is nullptr");
	}
}

void ModelManager::CheckAssimpOption()
{
	Assimp::Importer importer;

	std::string extensions;
	importer.GetExtensionList(extensions);

	Log::Write(LogLevel::Info, "Assimp Format List" + extensions);

	Log::Write(LogLevel::Info, "Assimp Version: "
		+ std::to_string(aiGetVersionMajor()) + "."
		+ std::to_string(aiGetVersionMinor()) + "."
		+ std::to_string(aiGetVersionRevision()));
}

void ModelManager::CreateDefaultMesh()
{
	// Cube
	CreateCube();
	// Sphere
	CreateSphere();
	// Plane
	CreatePlane();
	// Ring
	CreateRing();
	// Cylinder
	CreateCylinder();
	// Skybox
	CreateSkybox();
	// EffectRing
	CreateEffectRing();
}

void ModelManager::CreateCube()
{
	// Cube
	std::wstring modelName = L"Cube";
	modelName = GenerateUniqueName(modelName, m_ModelNameContainer);
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
	meshData.vertices[0] = { {0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},{1.0f,1.0f,1.0f,1.0f},{0} }; // 右上
	meshData.vertices[1] = { {0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{1} }; // 左上
	meshData.vertices[2] = { {0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{2} }; // 右下
	meshData.vertices[3] = { {0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},{1.0f,1.0f,1.0f,1.0f},{3} }; // 左下

	// 左面
	meshData.vertices[4] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{4} }; // 左上
	meshData.vertices[5] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{5} }; // 右上
	meshData.vertices[6] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{6} }; // 左下
	meshData.vertices[7] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{7} }; // 右下

	// 前面
	meshData.vertices[8] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{8} }; // 左上
	meshData.vertices[9] = { { 0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{9} }; // 右上
	meshData.vertices[10] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{10} }; // 左下
	meshData.vertices[11] = { { 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{11} }; // 右下

	// 後面
	meshData.vertices[12] = { { 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} ,{1.0f,1.0f,1.0f,1.0f},{12} }; // 右上
	meshData.vertices[13] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} ,{1.0f,1.0f,1.0f,1.0f},{13} }; // 左上
	meshData.vertices[14] = { { 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} ,{1.0f,1.0f,1.0f,1.0f},{14} }; // 右下
	meshData.vertices[15] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},{1.0f,1.0f,1.0f,1.0f},{15} }; // 左下

	// 上面
	meshData.vertices[16] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{16} }; // 左奥
	meshData.vertices[17] = { { 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{17} }; // 右奥
	meshData.vertices[18] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{18} }; // 左前
	meshData.vertices[19] = { { 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{19} }; // 右前

	// 下面
	meshData.vertices[20] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{20} }; // 左前
	meshData.vertices[21] = { { 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{21} }; // 右前
	meshData.vertices[22] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{22} }; // 左奥
	meshData.vertices[23] = { { 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{23} }; // 右奥

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
	// コンテナに追加
	modelData.meshes.push_back(meshData);
	// modelDataを追加
	AddModelData(modelData);
}

void ModelManager::CreateSphere()
{
	// Sphere
	std::wstring modelName = L"Sphere";
	modelName = GenerateUniqueName(modelName, m_ModelNameContainer);
	ModelData modelData;
	modelData.name = modelName;
	MeshData meshData;
	meshData.name = modelName;
	// 頂点数とインデックス数
	uint32_t kSubdivision = 32; // 分割数
	float kLonEvery = 2.0f * PI / kSubdivision;
	float kLatEvery = PI / kSubdivision;
	uint32_t vertices = kSubdivision * kSubdivision * 4;
	uint32_t indices = kSubdivision * kSubdivision * 6;
	// メモリ確保
	meshData.vertices.resize(vertices);
	meshData.indices.resize(indices);
	// 頂点データを設定
#pragma region
	// 緯度の方向に分割　-π/2 ～ π/2
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex)
	{
		float lat = -PI / 2.0f + kLatEvery * latIndex; // 現在の緯度

		// 経度の方向に分割 0 ～ 2π
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex)
		{
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 4;;
			float lon = lonIndex * kLonEvery; // 現在の経度
			// 頂点にデータを入力する
			// A頂点
			meshData.vertices[start].position.x = cos(lat) * cos(lon);
			meshData.vertices[start].position.y = sin(lat);
			meshData.vertices[start].position.z = cos(lat) * sin(lon);
			meshData.vertices[start].position.w = 1.0f;
			meshData.vertices[start].texCoord = { float(lonIndex) / float(kSubdivision),1.0f - float(latIndex) / float(kSubdivision) };
			meshData.vertices[start].normal.x = meshData.vertices[start].position.x;
			meshData.vertices[start].normal.y = meshData.vertices[start].position.y;
			meshData.vertices[start].normal.z = meshData.vertices[start].position.z;
			meshData.vertices[start].color = { 1.0f,1.0f,1.0f,1.0f };
			meshData.vertices[start].vertexID = start;
			// B頂点
			meshData.vertices[start + 1].position.x = cos(lat + kLatEvery) * cos(lon);
			meshData.vertices[start + 1].position.y = sin(lat + kLatEvery);
			meshData.vertices[start + 1].position.z = cos(lat + kLatEvery) * sin(lon);
			meshData.vertices[start + 1].position.w = 1.0f;
			meshData.vertices[start + 1].texCoord = { float(lonIndex) / float(kSubdivision),1.0f - float(latIndex + 1) / float(kSubdivision) };
			meshData.vertices[start + 1].normal.x = meshData.vertices[start + 1].position.x;
			meshData.vertices[start + 1].normal.y = meshData.vertices[start + 1].position.y;
			meshData.vertices[start + 1].normal.z = meshData.vertices[start + 1].position.z;
			meshData.vertices[start + 1].color = { 1.0f,1.0f,1.0f,1.0f };
			meshData.vertices[start + 1].vertexID = start + 1;
			// C頂点
			meshData.vertices[start + 2].position.x = cos(lat) * cos(lon + kLonEvery);
			meshData.vertices[start + 2].position.y = sin(lat);
			meshData.vertices[start + 2].position.z = cos(lat) * sin(lon + kLonEvery);
			meshData.vertices[start + 2].position.w = 1.0f;
			meshData.vertices[start + 2].texCoord = { float(lonIndex + 1) / float(kSubdivision),1.0f - float(latIndex) / float(kSubdivision) };
			meshData.vertices[start + 2].normal.x = meshData.vertices[start + 2].position.x;
			meshData.vertices[start + 2].normal.y = meshData.vertices[start + 2].position.y;
			meshData.vertices[start + 2].normal.z = meshData.vertices[start + 2].position.z;
			meshData.vertices[start + 2].color = { 1.0f,1.0f,1.0f,1.0f };
			meshData.vertices[start + 2].vertexID = start + 2;
			// D頂点
			meshData.vertices[start + 3].position.x = cos(lat + kLatEvery) * cos(lon + kLonEvery);
			meshData.vertices[start + 3].position.y = sin(lat + kLatEvery);
			meshData.vertices[start + 3].position.z = cos(lat + kLatEvery) * sin(lon + kLonEvery);
			meshData.vertices[start + 3].position.w = 1.0f;
			meshData.vertices[start + 3].texCoord = { float(lonIndex + 1) / float(kSubdivision),1.0f - float(latIndex + 1) / float(kSubdivision) };
			meshData.vertices[start + 3].normal.x = meshData.vertices[start + 3].position.x;
			meshData.vertices[start + 3].normal.y = meshData.vertices[start + 3].position.y;
			meshData.vertices[start + 3].normal.z = meshData.vertices[start + 3].position.z;
			meshData.vertices[start + 3].color = { 1.0f,1.0f,1.0f,1.0f };
			meshData.vertices[start + 3].vertexID = start + 3;

		}
	}
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex)
	{
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex)
		{
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 4;
			meshData.indices[start] = 0 + startIndex;
			meshData.indices[start + 1] = 1 + startIndex;
			meshData.indices[start + 2] = 2 + startIndex;
			meshData.indices[start + 3] = 1 + startIndex;
			meshData.indices[start + 4] = 3 + startIndex;
			meshData.indices[start + 5] = 2 + startIndex;
		}
	}
#pragma endregion
	// コンテナに追加
	modelData.meshes.push_back(meshData);
	// modelDataを追加
	AddModelData(modelData);
}

void ModelManager::CreatePlane()
{
	// Plane
	std::wstring modelName = L"Plane";
	modelName = GenerateUniqueName(modelName, m_ModelNameContainer);
	ModelData modelData;
	modelData.name = modelName;
	MeshData meshData;
	meshData.name = modelName;
	// 頂点数とインデックス数
	uint32_t vertices = 4;// 頂点数
	uint32_t indices = 6;// インデックス数
	// メモリ確保
	meshData.vertices.resize(vertices);
	meshData.indices.resize(indices);
	// 頂点データを設定
#pragma region
	// 頂点データ（重複なし）
	meshData.vertices[0] = { { 1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },{1.0f,1.0f,1.0f,1.0f},{0} };  // 右上
	meshData.vertices[1] = { {-1.0f,  1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } ,{1.0f,1.0f,1.0f,1.0f},{1} };  // 左上
	meshData.vertices[2] = { { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } ,{1.0f,1.0f,1.0f,1.0f},{2} };  // 右下
	meshData.vertices[3] = { {-1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f },{1.0f,1.0f,1.0f,1.0f},{3} };  // 左下
	// インデックスデータ
	meshData.indices[0] = 0;
	meshData.indices[1] = 1;
	meshData.indices[2] = 2;
	meshData.indices[3] = 1;
	meshData.indices[4] = 3;
	meshData.indices[5] = 2;
#pragma endregion
	// コンテナに追加
	modelData.meshes.push_back(meshData);
	// modelDataを追加
	AddModelData(modelData);
}

void ModelManager::CreateRing()
{
	// Ring
	std::wstring modelName = L"Ring";
	modelName = GenerateUniqueName(modelName, m_ModelNameContainer);
	ModelData modelData;
	modelData.name = modelName;
	MeshData meshData;
	meshData.name = modelName;
	// 頂点数とインデックス数
	const uint32_t kRingDivide = 32; // 分割数
	const float kOuterRadius = 1.0f; // 外半径
	const float kInnerRadius = 0.2f; // 内半径
	const float radianPerDivide = 2.0f * PI / static_cast<float>(kRingDivide); // 分割あたりのラジアン
	// 各分割セグメントは4つの頂点（外側2点、内側2点）を持つ
	uint32_t vertices = kRingDivide * 4;
	// 各分割セグメントは2つの三角形（6つのインデックス）で構成される
	uint32_t indices = kRingDivide * 6;
	// メモリ確保
	meshData.vertices.resize(vertices);
	meshData.indices.resize(indices);
	// 頂点データを設定
#pragma region
	// 頂点データ（重複なし）
	for (uint32_t i = 0; i < kRingDivide; ++i)
	{
		float currentAngle = i * radianPerDivide;
		float nextAngle = (i + 1) * radianPerDivide; // 次の角度（最後のセグメントでは (kRingDivide * radianPerDivide) = 2*Pi となるが、sin/cos の周期性により問題ない）

		float sinCurrent = std::sin(currentAngle);
		float cosCurrent = std::cos(currentAngle);
		float sinNext = std::sin(nextAngle);
		float cosNext = std::cos(nextAngle);

		// UV座標のU値
		// リングをテクスチャで一周させる場合、i/kRingDivide は適切
		// セグメントごとにテクスチャを貼る場合は調整が必要かもしれない
		float uCurrent = static_cast<float>(i) / static_cast<float>(kRingDivide);
		float uNext = static_cast<float>(i + 1) / static_cast<float>(kRingDivide);
		if (i == kRingDivide - 1)
		{ // 最後のセグメントのU座標を調整してシームレスにする場合
			uNext = 1.0f; // または uCurrent + (1.0f / kRingDivide) を使うなど、設計による
		}


		// 現在のセグメントの4つの頂点のインデックス
		uint32_t v0_outer_current = i * 4 + 0; // 外側、現在の角度
		uint32_t v1_outer_next = i * 4 + 1; // 外側、次の角度
		uint32_t v2_inner_current = i * 4 + 2; // 内側、現在の角度
		uint32_t v3_inner_next = i * 4 + 3; // 内側、次の角度

		// 頂点座標とUV座標、法線、色、カスタムインデックスを設定
		// 外側の円周上の点 (現在の角度)
		meshData.vertices[v0_outer_current] = { {cosCurrent * kOuterRadius, sinCurrent * kOuterRadius, 0.0f, 1.0f}, {uCurrent, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {v0_outer_current} };
		// 外側の円周上の点 (次の角度)
		meshData.vertices[v1_outer_next] = { {cosNext * kOuterRadius,    sinNext * kOuterRadius,    0.0f, 1.0f}, {uNext,    0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {v1_outer_next} };
		// 内側の円周上の点 (現在の角度)
		meshData.vertices[v2_inner_current] = { {cosCurrent * kInnerRadius, sinCurrent * kInnerRadius, 0.0f, 1.0f}, {uCurrent, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {v2_inner_current} };
		// 内側の円周上の点 (次の角度)
		meshData.vertices[v3_inner_next] = { {cosNext * kInnerRadius,    sinNext * kInnerRadius,    0.0f, 1.0f}, {uNext,    1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {v3_inner_next} };
	}
	
	// インデックスデータ
	for (uint32_t i = 0; i < kRingDivide; ++i)
	{
		// 現在のセグメントを構成する4つの頂点の開始インデックス
		uint32_t baseVertexIndex = i * 4;
		// 現在のセグメントのインデックスデータの書き込み開始位置
		uint32_t baseIndex = i * 6;

		// 頂点インデックス
		// v0: 外側、現在の角度
		// v1: 外側、次の角度
		// v2: 内側、現在の角度
		// v3: 内側、次の角度
		uint32_t v0 = baseVertexIndex + 0;
		uint32_t v1 = baseVertexIndex + 1;
		uint32_t v2 = baseVertexIndex + 2;
		uint32_t v3 = baseVertexIndex + 3;

		// 1つ目の三角形 (外側現在、外側次、内側現在)
		meshData.indices[baseIndex + 0] = v0;
		meshData.indices[baseIndex + 1] = v1;
		meshData.indices[baseIndex + 2] = v2;

		// 2つ目の三角形 (外側次、内側次、内側現在)
		meshData.indices[baseIndex + 3] = v1;
		meshData.indices[baseIndex + 4] = v3;
		meshData.indices[baseIndex + 5] = v2;
	}
	
#pragma endregion
	// コンテナに追加
	modelData.meshes.push_back(meshData);
	// modelDataを追加
	AddModelData(modelData);
}

void ModelManager::CreateCylinder()
{
	// Cylinder
	std::wstring modelName = L"Cylinder";
	modelName = GenerateUniqueName(modelName, m_ModelNameContainer);
	ModelData modelData;
	modelData.name = modelName;
	MeshData meshData;
	meshData.name = modelName;

	// パラメータ
	const uint32_t kCylinderDivide = 32;     // 分割数
	const float kTopRadius = 1.0f;           // 上部半径
	const float kBottomRadius = 1.0f;        // 下部半径
	const float kHeight = 3.0f;              // 高さ
	const float radianPerDivide = 2.0f * PI / static_cast<float>(kCylinderDivide);

	// 頂点数とインデックス数（側面のみ）
	const uint32_t vertices = kCylinderDivide * 4;
	const uint32_t indices = kCylinderDivide * 6;

	meshData.vertices.resize(vertices);
	meshData.indices.resize(indices);

	for (uint32_t i = 0; i < kCylinderDivide; ++i)
	{
		float angle = i * radianPerDivide;
		float nextAngle = (i + 1) * radianPerDivide;

		float sin = std::sin(angle);
		float cos = std::cos(angle);
		float sinNext = std::sin(nextAngle);
		float cosNext = std::cos(nextAngle);

		float u = static_cast<float>(i) / static_cast<float>(kCylinderDivide);
		float uNext = static_cast<float>(i + 1) / static_cast<float>(kCylinderDivide);

		// 頂点インデックス
		uint32_t v0 = i * 4 + 0; // 上部現在
		uint32_t v1 = i * 4 + 1; // 上部次
		uint32_t v2 = i * 4 + 2; // 下部現在
		uint32_t v3 = i * 4 + 3; // 下部次

		// 頂点設定
		meshData.vertices[v0] = { { -sin * kTopRadius,     kHeight, cos * kTopRadius,     1.0f }, { u,     0.0f }, { -sin, 0.0f, cos }, { 1,1,1,1 }, { v0 } };
		meshData.vertices[v1] = { { -sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f }, { uNext, 0.0f }, { -sinNext, 0.0f, cosNext }, { 1,1,1,1 }, { v1 } };
		meshData.vertices[v2] = { { -sin * kBottomRadius,     0.0f, cos * kBottomRadius,     1.0f }, { u,     1.0f }, { -sin, 0.0f, cos }, { 1,1,1,1 }, { v2 } };
		meshData.vertices[v3] = { { -sinNext * kBottomRadius, 0.0f, cosNext * kBottomRadius, 1.0f }, { uNext, 1.0f }, { -sinNext, 0.0f, cosNext }, { 1,1,1,1 }, { v3 } };

		// インデックス設定（三角形2つ）
		uint32_t indexBase = i * 6;
		meshData.indices[indexBase + 0] = v0;
		meshData.indices[indexBase + 1] = v1;
		meshData.indices[indexBase + 2] = v2;

		meshData.indices[indexBase + 3] = v1;
		meshData.indices[indexBase + 4] = v3;
		meshData.indices[indexBase + 5] = v2;
	}

	// メッシュ追加
	modelData.meshes.push_back(meshData);
	AddModelData(modelData);
}

void ModelManager::CreateSkybox()
{
	// Skybox
	std::wstring modelName = L"Skybox";
	modelName = GenerateUniqueName(modelName, m_ModelNameContainer);
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
	meshData.vertices[0] = { {0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},{1.0f,1.0f,1.0f,1.0f},{0} }; // 右上
	meshData.vertices[1] = { {0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{1} }; // 左上
	meshData.vertices[2] = { {0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{2} }; // 右下
	meshData.vertices[3] = { {0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},{1.0f,1.0f,1.0f,1.0f},{3} }; // 左下

	// 左面
	meshData.vertices[4] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{4} }; // 左上
	meshData.vertices[5] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{5} }; // 右上
	meshData.vertices[6] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{6} }; // 左下
	meshData.vertices[7] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{7} }; // 右下

	// 前面
	meshData.vertices[8] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{8} }; // 左上
	meshData.vertices[9] = { { 0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{9} }; // 右上
	meshData.vertices[10] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{10} }; // 左下
	meshData.vertices[11] = { { 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} ,{1.0f,1.0f,1.0f,1.0f},{11} }; // 右下

	// 後面
	meshData.vertices[12] = { { 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} ,{1.0f,1.0f,1.0f,1.0f},{12} }; // 右上
	meshData.vertices[13] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} ,{1.0f,1.0f,1.0f,1.0f},{13} }; // 左上
	meshData.vertices[14] = { { 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} ,{1.0f,1.0f,1.0f,1.0f},{14} }; // 右下
	meshData.vertices[15] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},{1.0f,1.0f,1.0f,1.0f},{15} }; // 左下

	// 上面
	meshData.vertices[16] = { {-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{16} }; // 左奥
	meshData.vertices[17] = { { 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{17} }; // 右奥
	meshData.vertices[18] = { {-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{18} }; // 左前
	meshData.vertices[19] = { { 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{19} }; // 右前

	// 下面
	meshData.vertices[20] = { {-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{20} }; // 左前
	meshData.vertices[21] = { { 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{21} }; // 右前
	meshData.vertices[22] = { {-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{22} }; // 左奥
	meshData.vertices[23] = { { 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} ,{1.0f,1.0f,1.0f,1.0f},{23} }; // 右奥

	// 右面インデックス
	meshData.indices[0] = 0; meshData.indices[1] = 1; meshData.indices[2] = 2;
	meshData.indices[3] = 2; meshData.indices[4] = 1; meshData.indices[5] = 3;

	// 左面インデックス
	meshData.indices[6] = 4; meshData.indices[7] = 5; meshData.indices[8] = 6;
	meshData.indices[9] = 6; meshData.indices[10] = 5; meshData.indices[11] = 7;

	// 前面インデックス
	meshData.indices[12] = 8; meshData.indices[13] = 9; meshData.indices[14] = 10;
	meshData.indices[15] = 10; meshData.indices[16] = 9; meshData.indices[17] = 11;

	// 後面インデックス
	meshData.indices[18] = 12; meshData.indices[19] = 13; meshData.indices[20] = 14;
	meshData.indices[21] = 14; meshData.indices[22] = 13; meshData.indices[23] = 15;

	// 上面インデックス
	meshData.indices[24] = 16; meshData.indices[25] = 17; meshData.indices[26] = 18;
	meshData.indices[27] = 18; meshData.indices[28] = 17; meshData.indices[29] = 19;

	// 下面インデックス
	meshData.indices[30] = 20; meshData.indices[31] = 21; meshData.indices[32] = 22;
	meshData.indices[33] = 22; meshData.indices[34] = 21; meshData.indices[35] = 23;
#pragma endregion
	// コンテナに追加
	modelData.meshes.push_back(meshData);
	// modelDataを追加
	AddModelData(modelData);
}

void ModelManager::CreateEffectRing()
{
	// Ring
	m_EffectRingMeshData.name = L"EffectRing";
	const uint32_t kOuterVertexCount = 256;
	uint32_t vertices = kOuterVertexCount * 4;
	uint32_t indices = kOuterVertexCount * 6;
	m_EffectRingMeshData.vertices.resize(vertices, VertexData());
	m_EffectRingMeshData.indices.resize(indices);

	for (uint32_t i = 0; i < kOuterVertexCount; ++i)
	{
		uint32_t base = i * 4;
		uint32_t index = i * 6;

		m_EffectRingMeshData.indices[index + 0] = base + 0;
		m_EffectRingMeshData.indices[index + 1] = base + 1;
		m_EffectRingMeshData.indices[index + 2] = base + 2;

		m_EffectRingMeshData.indices[index + 3] = base + 1;
		m_EffectRingMeshData.indices[index + 4] = base + 3;
		m_EffectRingMeshData.indices[index + 5] = base + 2;
	}

	// VertexBuffer,IndexBuffer作成
	m_EffectRingMeshData.vertexBufferIndex = m_pResourceManager->CreateVertexBuffer<VertexData>(static_cast<UINT>(m_EffectRingMeshData.vertices.size()));
	m_EffectRingMeshData.indexBufferIndex = m_pResourceManager->CreateIndexBuffer<uint32_t>(static_cast<UINT>(m_EffectRingMeshData.indices.size()));
	VertexBuffer<VertexData>* vertexBuffer = dynamic_cast<VertexBuffer<VertexData>*>(m_pResourceManager->GetBuffer<IVertexBuffer>(m_EffectRingMeshData.vertexBufferIndex));
	IndexBuffer<uint32_t>* indexBuffer = dynamic_cast<IndexBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IIndexBuffer>(m_EffectRingMeshData.indexBufferIndex));
	// コピー
	vertexBuffer->MappedDataCopy(m_EffectRingMeshData.vertices);
	indexBuffer->MappedDataCopy(m_EffectRingMeshData.indices);
}


Node ModelManager::ReadNode(aiNode* node,const std::string& parentName)
{
	Node result;
	Node result2;
	aiMatrix4x4 aiLocalMatrix = node->mTransformation;// nodeのlocalMatrixを取得
	aiLocalMatrix.Transpose();// 列ベクトルを行ベクトルに転置
	for (uint32_t mindex = 0; mindex < 4; ++mindex)
	{
		for (uint32_t index = 0; index < 4; ++index)
		{
			result2.localMatrix.m[mindex][index] = aiLocalMatrix[mindex][index];
		}
	}
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);// assimpの行列からSRTを抽出する関数を利用
	result.transform.scale = { scale.x,scale.y,scale.z };// scaleはそのまま
	result.transform.rotation = { rotate.x,-rotate.y,-rotate.z,rotate.w };// x軸を反転。さらに回転方向が逆なので軸を反転させる
	result.transform.translation = { -translate.x,translate.y,translate.z };// x軸を反転
	result.localMatrix = Theatria::Math::MakeAffineMatrix(result.transform.scale, result.transform.rotation, result.transform.translation);
	result.name = node->mName.C_Str();// Node名を格納
	result.parentName = parentName;// 親Node名を格納
	result.children.resize(node->mNumChildren);// 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		// 再帰的に読んで階層構造を作っていく
		result.children[childIndex] = ReadNode(node->mChildren[childIndex],result.name);
	}
	return result;
}

int32_t ModelManager::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = float4x4::Identity();
	joint.transform = node.transform;
	joint.index = static_cast<int32_t>(joints.size());// 現在登録されている数をIndexに
	joint.parent = parent;
	joints.push_back(joint);// SkeletonのJoint列に追加
	for (const Node& child : node.children)
	{
		// 子Jointを作成し、そのIndexを登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	// 自身のIndexを返す
	return joint.index;
	/*
	本来はanimationするNodeのみを対象にしたほうがいいが今は全Nodeを対象にしている
	*/
}

// ModelDataの追加
void ModelManager::AddModelData(ModelData& modelData)
{
	for (MeshData& mesh : modelData.meshes)
	{
		// VertexBuffer,IndexBuffer作成
		mesh.vertexBufferIndex = m_pResourceManager->CreateVertexBuffer<VertexData>(static_cast<UINT>(mesh.vertices.size()));
		mesh.indexBufferIndex = m_pResourceManager->CreateIndexBuffer<uint32_t>(static_cast<UINT>(mesh.indices.size()));
		VertexBuffer<VertexData>* vertexBuffer = dynamic_cast<VertexBuffer<VertexData>*>(m_pResourceManager->GetBuffer<IVertexBuffer>(mesh.vertexBufferIndex));
		IndexBuffer<uint32_t>* indexBuffer = dynamic_cast<IndexBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IIndexBuffer>(mesh.indexBufferIndex));
		// コピー
		vertexBuffer->MappedDataCopy(mesh.vertices);
		indexBuffer->MappedDataCopy(mesh.indices);

		mesh.skinInfoBufferIndex = m_pResourceManager->CreateConstantBuffer<SkinningInformation>();
		ConstantBuffer<SkinningInformation>* skinInfoBuffer = dynamic_cast<ConstantBuffer<SkinningInformation>*>(m_pResourceManager->GetBuffer<IConstantBuffer>(mesh.skinInfoBufferIndex));
		// SkinningInformationの初期化
		SkinningInformation skinInfo;
		skinInfo.numVertices = static_cast<uint32_t>(mesh.vertices.size());
		if (modelData.isBone)
		{
			skinInfo.boneCount = static_cast<uint32_t>(modelData.skeleton.joints.size());
			skinInfo.isSkinned = 1; // スキニングを有効にする
			skinInfoBuffer->UpdateData(skinInfo);
		}
		else {
			skinInfo.boneCount = 0; // ボーンがない場合は0
			skinInfo.isSkinned = 0; // スキニングを無効にする
			skinInfoBuffer->UpdateData(skinInfo);
		}
		mesh.influenceBufferIndex = m_pResourceManager->CreateStructuredBuffer<ConstBufferDataVertexInfluence>(static_cast<UINT>(mesh.vertices.size())); // 影響度バッファを作成（最大4つのボーン影響を持つと仮定）
		if (modelData.isBone)
		{
			StructuredBuffer<ConstBufferDataVertexInfluence>* influenceBuffer = dynamic_cast<StructuredBuffer<ConstBufferDataVertexInfluence>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(mesh.influenceBufferIndex));
			// 影響度バッファの初期化
			std::span<ConstBufferDataVertexInfluence> influences = influenceBuffer->GetMappedData();
			std::memcpy(influences.data(), mesh.skinCluster.influenceData.data.data(), sizeof(ConstBufferDataVertexInfluence) * influences.size());
		}
	}
	// UseTransformのリソースを作成
	modelData.useTransformBufferIndex = m_pResourceManager->CreateStructuredBuffer<uint32_t>(kUseTransformOffset);
	// ボーン行列統合バッファ
	UINT jointCount = 0;
	if (modelData.skeleton.joints.empty())
	{
		jointCount = 1;
	}
	else
	{
		jointCount = static_cast<UINT>(modelData.skeleton.joints.size() * 100);
	}
	modelData.boneMatrixBufferIndex = m_pResourceManager->CreateStructuredBuffer<ConstBufferDataWell>(jointCount);
	// 名前が重複していたら、エラー
	// ここに処理を追加する

	//// test
	//// コマンド引数バッファを作成
	//UINT64 byteStride =
	//	8 /*cbv*/ + 8 /*srv*/ + 8 /*srv*/ + 8 /*srv*/ + 
	//	8 /*srv*/ + 8 /*cbv*/ + 8 /*cbv*/ + 8 /*cbv*/ + 
	//	8 /*srv*/ + 8 /*srv*/ +
	//	sizeof(D3D12_VERTEX_BUFFER_VIEW) + // 16
	//	sizeof(D3D12_INDEX_BUFFER_VIEW) + // 16
	//	sizeof(D3D12_DRAW_INDEXED_ARGUMENTS); // 20
	//const UINT maxCmdCount = 1;
	//const UINT64 bufferSize = byteStride * maxCmdCount;
	//modelData.argsBuffer.byteStride = byteStride;
	//modelData.argsBuffer.totalBytes = bufferSize;
	//modelData.argsBuffer.h_Upload = std::make_unique<GpuBuffer>();
	//D3D12_HEAP_PROPERTIES heapProps = {};// ヒープの設定
	//heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;// CPUから書き込み可能
	//// Bufferの作成
	//modelData.argsBuffer.h_Upload->CreateBuffer(
	//	m_pResourceManager->GetDevice(),// デバイス
	//	heapProps,// ヒープの設定
	//	D3D12_HEAP_FLAG_NONE,// ヒープのフラグ
	//	D3D12_RESOURCE_STATE_GENERIC_READ,// リソースの状態
	//	D3D12_RESOURCE_FLAG_NONE,// リソースのフラグ
	//	1, static_cast<UINT>(bufferSize));
	//// マッピング
	//IndirectArgsRecord* mappedData = nullptr;// 一時マップ用
	//size_t mappedDataSize = maxCmdCount;
	//modelData.argsBuffer.h_Upload->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
	//// spanでラップ
	//modelData.argsBuffer.mappedData = std::span<IndirectArgsRecord>(mappedData, mappedDataSize);

	//// Default
	//modelData.argsBuffer.h_Default = std::make_unique<GpuBuffer>();
	//heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;// GPU専用
	//// Bufferの作成
	//modelData.argsBuffer.h_Default->CreateBuffer(
	//	m_pResourceManager->GetDevice(),// デバイス
	//	heapProps,// ヒープの設定
	//	D3D12_HEAP_FLAG_NONE,// ヒープのフラグ
	//	D3D12_RESOURCE_STATE_COMMON,// リソースの状態
	//	D3D12_RESOURCE_FLAG_NONE,// リソースのフラグ
	//	1, static_cast<UINT>(bufferSize));


	// モデルをコンテナに追加
	std::wstring name = modelData.name;
	uint32_t index = static_cast<uint32_t>(m_Models.push_back(std::move(modelData)));
	// 名前コンテナに登録
	m_ModelNameContainer[name] = index;
}
