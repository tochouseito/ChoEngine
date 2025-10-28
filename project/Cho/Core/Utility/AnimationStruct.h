#pragma once
#include "ChoMath.h"
#include <vector>
#include <map>
#include <optional>

using namespace Theatria::Math;

// Node用Transform構造体
struct NodeTransform
{
	float3 translation = { 0.0f, 0.0f, 0.0f };
	Quaternion rotation = { 0.0f, 0.0f, 0.0f,1.0f };
	Scale scale = { 1.0f, 1.0f, 1.0f };
};

const uint32_t kNumMaxInfluence = 4;

template<typename T>
struct Keyframe
{
	float time = 0.0f;
	T value;
};
using Keyframefloat3 = Keyframe<float3>;
using KeyframeQuaternion = Keyframe<Quaternion>;
using KeyframeScale = Keyframe<Scale>;

template<typename T>
struct AnimationCurve
{
	std::vector<Keyframe<T>> keyframes;
};
struct NodeAnimation
{
	AnimationCurve<Scale> scale;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<float3> translate;
};

// ジョイント
struct Joint
{
	NodeTransform transform;// Transform情報
	float4x4 localMatrix;
	float4x4 skeletonSpaceMatrix;// skeletonSpaceでの変換行列
	std::string name;// 名前
	std::vector<int32_t> children;// 子JointのIndexのリスト。いなければ空
	int32_t index = 0;// 自身のIndex
	std::optional<int32_t> parent;// 親JointのIndex。いなければnull
};

// スケルトン
struct Skeleton
{
	int32_t root = 0;// RootJointのIndex
	std::map<std::string, int32_t> jointMap;// Joint名とIndexとの辞書
	std::vector<Joint> joints;// 所属しているJoint
};

// ウェイトデータ
struct VertexWeightData
{
	float weight;
	uint32_t vertexIndex;
};
struct JointWeightData
{
	float4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};
// skinning
struct SkinningInformation
{
	uint32_t numVertices;
	uint32_t boneCount;
	uint32_t isSkinned = 0; // 0: スキニングなし, 1: スキニングあり
};
struct ConstBufferDataVertexInfluence
{
	std::array<float, kNumMaxInfluence>weights;
	std::array<int32_t, kNumMaxInfluence>jointIndices;
};
struct InfluenceData
{
	std::vector<ConstBufferDataVertexInfluence> data;
	//uint32_t bufferIndex = 0;
};
struct ConstBufferDataWell
{
	float4x4 skeletonSpaceMatrix;// 位置用
	float4x4 skeletonSpaceInverseTransposeMatrix;// 法線用
};
struct PaletteData
{
	std::vector<ConstBufferDataWell> data;
	//uint32_t bufferIndex = 0;
};
struct SkinCluster
{
	std::vector<float4x4> inverseBindPoseMatrices;
	PaletteData paletteData;
	InfluenceData influenceData;
	//uint32_t skinningBufferIndex = 0;

	// コピー演算子
	/*SkinCluster& operator=(const SkinCluster& other)
	{
		if (this == &other) return *this;
		inverseBindPoseMatrices = other.inverseBindPoseMatrices;
		paletteData = other.paletteData;
		influenceData = other.influenceData;
		return *this;
	}*/
};
