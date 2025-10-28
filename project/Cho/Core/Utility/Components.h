#pragma once
#include "ChoMath.h"// TheatriaEngine数学ライブラリ
#include "Core/Utility/Color.h"
#include "Core/Utility/IDType.h"
#include "Core/Utility/EffectStruct.h"
#include "Core/Utility/AnimationStruct.h"
#include "Core/Utility/SoundData.h"
#include "GameCore/ECS/ECSManager.h" // ECSManager
#include <array>          // C++11
#include <unordered_set>  // C++11
#include <typeindex>      // C++11
#include <optional>       // C++17
#include <ranges>         // C++20
#include <numbers>        // C++20
#include <variant>

#include <2D/d2_common.h>
#include <3D/d3_common.h>

class GameObject;

// 初期値を保存するための構造体
struct TransformStartValue
{
	float3 translation = { 0.0f, 0.0f, 0.0f };
	Quaternion rotation = { 0.0f, 0.0f, 0.0f,1.0f };
	Scale scale = { 1.0f, 1.0f, 1.0f };
	float3 degrees = { 0.0f, 0.0f, 0.0f };
};

struct TransformComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	TransformComponent() = default;
	// コピーコンストラクタ
	TransformComponent(const TransformComponent&) = default;
	// ムーブコンストラクタ（移動）
	TransformComponent(TransformComponent&&) noexcept = default;

	float3 position = { 0.0f, 0.0f, 0.0f };			// 位置
	Quaternion quaternion = { 0.0f, 0.0f, 0.0f,1.0f };	// 回転
	Scale scale = { 1.0f, 1.0f, 1.0f };					// スケール
    float4x4 matWorld = float4x4::Identity();		// ワールド行列
	float4x4 rootMatrix = float4x4::Identity();	// ルートのワールド行列
	float3 degrees = { 0.0f,0.0f,0.0f };				// 度数表示,操作用変数
	float3 prePos = { 0.0f,0.0f,0.0f };				// 位置差分計算用
	float3 preRot = { 0.0f,0.0f,0.0f };				// 回転差分計算用
	Scale preScale = { 1.0f,1.0f,1.0f };				// スケール差分計算用
	std::wstring parentName = L"";						// 親の名前
	std::vector<std::wstring> childNames;				// 子供の名前
	std::optional<uint32_t> parent = std::nullopt;		// 親のEntity
	float4x4 matLocal = float4x4::Identity();	// ローカル行列
	float4x4 matRotation = float4x4::Identity();	// 回転行列
	float4x4 matScale = float4x4::Identity();	// スケール行列
	int tickPriority = 0;								// Tick優先度
	float3 forward = { 0.0f, 0.0f, 1.0f };			// 前方向ベクトル
	std::optional<uint32_t> mapID = std::nullopt;		// マップインデックス
	TransformStartValue startValue;						// 初期値保存用
	std::optional<uint32_t> materialID = std::nullopt;	// マテリアルID
	std::optional<uint32_t> boneOffsetID = std::nullopt;	// ボーンオフセットID
	bool isBillboard = false;				// ビルボードフラグ

	TransformComponent& operator=(const TransformComponent& other)
	{
		if (this == &other) return *this;

		position = other.position;
		quaternion = other.quaternion;
		scale = other.scale;
		degrees = other.degrees;
		prePos = other.prePos;
		preRot = other.preRot;
		preScale = other.preScale;
		tickPriority = other.tickPriority;
		isBillboard = other.isBillboard;

		// optionalな要素は、コピー可能な値として処理
		parent = other.parent;
		//mapID = other.mapID;
		//materialID = other.materialID;

		// ワールド行列やルート行列は差分として扱わない場合はコピーしない
		// matWorld = other.matWorld;
		// rootMatrix = other.rootMatrix;

		// startValue も含めたい場合はこちらを有効に
		//startValue = other.startValue;

		return *this;
	}

	// ムーブ代入を明示的に生成
	TransformComponent& operator=(TransformComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		position.Initialize();
		quaternion.Initialize();
		scale.Initialize();
		matWorld = float4x4::Identity();
		rootMatrix = float4x4::Identity();
		degrees.Initialize();
		prePos.Initialize();
		preRot.Initialize();
		preScale.Initialize();
		tickPriority = 0;
		parent = std::nullopt;
		mapID = std::nullopt;
		materialID = std::nullopt;
		boneOffsetID = std::nullopt;
		isBillboard = false;
	}
};

struct CameraComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	CameraComponent() = default;
	// コピーコンストラクタ
	CameraComponent(const CameraComponent&) = default;
	// ムーブコンストラクタ（移動）
	CameraComponent(CameraComponent&&) noexcept = default;

    // 垂直方向視野角
    float fovAngleY = 45.0f * std::numbers::pi_v<float> / 180.0f;
    // ビューポートのアスペクト比
    float aspectRatio = 16.0f / 9.0f;
    // 深度限界（手前側）
    float nearZ = 0.1f;
    // 深度限界（奥側）
    float farZ = 1000.0f;
	// View行列
	float4x4 viewMatrix = float4x4::Identity();
	// Projection行列
	float4x4 projectionMatrix = float4x4::Identity();
	// バッファーインデックス
	std::optional<uint32_t> bufferIndex = std::nullopt;

	CameraComponent& operator=(const CameraComponent& other)
	{
		if (this == &other) return *this;
		fovAngleY = other.fovAngleY;
		aspectRatio = other.aspectRatio;
		nearZ = other.nearZ;
		farZ = other.farZ;
		return *this;
	}

	// ムーブ代入を明示的に生成
	CameraComponent& operator=(CameraComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		fovAngleY = 45.0f * std::numbers::pi_v<float> / 180.0f;
		aspectRatio = 16.0f / 9.0f;
		nearZ = 0.1f;
		farZ = 1000.0f;
	}
};

// メッシュコンポーネント
struct MeshFilterComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	MeshFilterComponent() = default;
	// コピーコンストラクタ
	MeshFilterComponent(const MeshFilterComponent&) = default;
	// ムーブコンストラクタ（移動）
	MeshFilterComponent(MeshFilterComponent&&) noexcept = default;

	std::wstring modelName = L"";// モデル名
	std::optional<uint32_t> modelID = std::nullopt;// Model選択用ID

	MeshFilterComponent& operator=(const MeshFilterComponent& other)
	{
		if (this == &other) return *this;
		modelName = other.modelName;
		return *this;
	}

	// ムーブ代入を明示的に生成
	MeshFilterComponent& operator=(MeshFilterComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		modelName = L"";
		modelID = std::nullopt;
	}
};

// 描画コンポーネント
struct SubMesh
{

};
struct MeshRendererComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	MeshRendererComponent() = default;
	// コピーコンストラクタ
	MeshRendererComponent(const MeshRendererComponent&) = default;
	// ムーブコンストラクタ（移動）
	MeshRendererComponent(MeshRendererComponent&&) noexcept = default;

	bool visible = true;// 描画フラグ

	MeshRendererComponent& operator=(const MeshRendererComponent& other)
	{
		if (this == &other) return *this;
		visible = other.visible;
		return *this;
	}

	// ムーブ代入を明示的に生成
	MeshRendererComponent& operator=(MeshRendererComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		visible = true;
	}
};

// スクリプトコンポーネント
class Marionnette;
struct ScriptComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	ScriptComponent() = default;
	// コピーコンストラクタ
	ScriptComponent(const ScriptComponent&) = default;
	// ムーブコンストラクタ（移動）
	ScriptComponent(ScriptComponent&&) noexcept = default;

	std::string scriptName = "";								// スクリプト名
	ObjectHandle objectHandle;			// スクリプトのオブジェクトハンドル
	using ScriptFunc = std::function<void()>;					// スクリプト関数型

	Marionnette* instance = nullptr;							// スクリプトインスタンス
	ScriptFunc startFunc;										// Start関数
	ScriptFunc updateFunc;										// Update関数
	std::function<void()> cleanupFunc;							// 解放関数

	std::function<void(GameObject& other)> onCollisionEnterFunc;// 衝突開始関数
	std::function<void(GameObject& other)> onCollisionStayFunc;	// 衝突中関数
	std::function<void(GameObject& other)> onCollisionExitFunc;	// 衝突終了関数

	bool isActive = false;				// スクリプト有効フラグ

	ScriptComponent& operator=(const ScriptComponent& other)
	{
		if (this == &other) return *this;
		scriptName = other.scriptName;
		return *this;
	}

	// ムーブ代入を明示的に生成
	ScriptComponent& operator=(ScriptComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		scriptName = "";
		objectHandle.Clear();
		startFunc = nullptr;
		updateFunc = nullptr;
		cleanupFunc = nullptr;
		onCollisionEnterFunc = nullptr;
		onCollisionStayFunc = nullptr;
		onCollisionExitFunc = nullptr;
		isActive = false;
	}
    struct FieldDesc
    {
        void* ptr = nullptr;
        std::type_index type = typeid(void);
        std::pair<uint32_t, uint32_t> minmax = { 0, 0 }; // for int, float
    };
    struct FieldVal  // 保存用
    {
        using Value = std::variant<int, float, float3, bool>;
        Value value;
        std::type_index type = typeid(void);
        std::pair<uint32_t, uint32_t> minmax = { 0, 0 };
    };
    std::unordered_map<std::string, FieldDesc> fields;
};

// ライン描画コンポーネント
struct LineData
{
	float3 start;	// 始点
	float3 end;	// 終点
	Color color;	// 色
};

struct LineRendererComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	LineRendererComponent() = default;
	// コピーコンストラクタ
	LineRendererComponent(const LineRendererComponent&) = default;
	// ムーブコンストラクタ（移動）
	LineRendererComponent(LineRendererComponent&&) noexcept = default;

	LineData line;// ラインデータ
	std::optional<uint32_t> mapID = std::nullopt;		// マップインデックス

	LineRendererComponent& operator=(const LineRendererComponent& other)
	{
		if (this == &other) return *this;
		line = other.line;
		return *this;
	}

	// ムーブ代入を明示的に生成
	LineRendererComponent& operator=(LineRendererComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		line.start.Initialize();
		line.end.Initialize();
		line.color.Initialize();
		mapID = std::nullopt;
	}
};

// 2D物理コンポーネント
// 物理エンジンに統合予定
// 2D物理挙動コンポーネント
struct Rigidbody2DComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	Rigidbody2DComponent() = default;
	// コピーコンストラクタ
	Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	// ムーブコンストラクタ（移動）
	Rigidbody2DComponent(Rigidbody2DComponent&&) noexcept = default;

	bool isActive = true; // 有効フラグ
	float mass = 1.0f;
	float gravityScale = 1.0f;
	bool isKinematic = false;
	bool fixedRotation = false;
	physics::d2::Id2BodyType bodyType = physics::d2::Id2BodyType::Id2_dynamicBody;
	physics::d2::Id2Body* runtimeBody = nullptr;
	bool isCollisionStay = false; // 衝突中フラグ
	std::optional<Entity> otherEntity = std::nullopt; // 衝突したオブジェクトID
	std::optional<Entity> selfEntity = std::nullopt; // 自分のオブジェクトID
	std::optional<float2> requestedPosition = std::nullopt; // 位置リクエスト
	//std::optional<b2Vec2> requestedVelocity = std::nullopt; // 速度リクエスト
	float2 velocity = { 0.0f, 0.0f }; // 速度

	Rigidbody2DComponent& operator=(const Rigidbody2DComponent& other)
	{
		if (this == &other) return *this;
		isActive = other.isActive;
		mass = other.mass;
		gravityScale = other.gravityScale;
		isKinematic = other.isKinematic;
		fixedRotation = other.fixedRotation;
		bodyType = other.bodyType;
		return *this;
	}

	// ムーブ代入を明示的に生成
	Rigidbody2DComponent& operator=(Rigidbody2DComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		isActive = true;
		mass = 1.0f;
		gravityScale = 1.0f;
		isKinematic = false;
		fixedRotation = false;
		bodyType = physics::d2::Id2_dynamicBody;
		isCollisionStay = false;
		otherEntity = std::nullopt;
		selfEntity = std::nullopt;
		requestedPosition = std::nullopt;
		runtimeBody = nullptr;
		velocity.Initialize();
	}
};
// 2D矩形コライダー
struct BoxCollider2DComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	BoxCollider2DComponent() = default;
	// コピーコンストラクタ
	BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	// ムーブコンストラクタ（移動）
	BoxCollider2DComponent(BoxCollider2DComponent&&) noexcept = default;

	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float width = 1.0f;
	float height = 1.0f;
	float preWidth = 1.0f; // 前回の幅
	float preHeight = 1.0f; // 前回の高さ
	float density = 1.0f;
	float friction = 0.3f;
	float restitution = 0.0f;
	bool isSensor = false;
	bool isActive = true;
	physics::d2::Id2Shape* runtimeShape = nullptr;

	BoxCollider2DComponent& operator=(const BoxCollider2DComponent& other)
	{
		if (this == &other) return *this;
		offsetX = other.offsetX;
		offsetY = other.offsetY;
		width = other.width;
		height = other.height;
		preWidth = other.preWidth;
		preHeight = other.preHeight;
		density = other.density;
		friction = other.friction;
		restitution = other.restitution;
		isSensor = other.isSensor;
		return *this;
	}

	// ムーブ代入を明示的に生成
	BoxCollider2DComponent& operator=(BoxCollider2DComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		offsetX = 0.0f;
		offsetY = 0.0f;
		width = 1.0f;
		height = 1.0f;
		density = 1.0f;
		friction = 0.3f;
		restitution = 0.0f;
		isSensor = false;
		isActive = true;
		runtimeShape = nullptr;
	}
};
// 2D円形コライダー
struct CircleCollider2DComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	CircleCollider2DComponent() = default;
	// コピーコンストラクタ
	CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	// ムーブコンストラクタ（移動）
	CircleCollider2DComponent(CircleCollider2DComponent&&) noexcept = default;

	// ムーブ代入を明示的に生成
	CircleCollider2DComponent& operator=(CircleCollider2DComponent&&) noexcept = default;

	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float radius = 0.5f;
	float density = 1.0f;
	float friction = 0.3f;
	float restitution = 0.0f;
};
// 2D任意形状コライダー
struct PolygonCollider2DComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	PolygonCollider2DComponent() = default;
	// コピーコンストラクタ
	PolygonCollider2DComponent(const PolygonCollider2DComponent&) = default;
	// ムーブコンストラクタ（移動）
	PolygonCollider2DComponent(PolygonCollider2DComponent&&) noexcept = default;

	// ムーブ代入を明示的に生成
	PolygonCollider2DComponent& operator=(PolygonCollider2DComponent&&) noexcept = default;
	float density = 1.0f;
	float friction = 0.3f;
	float restitution = 0.0f;
};

// 3D物理コンポーネント
struct Rigidbody3DComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	Rigidbody3DComponent() = default;
	// コピーコンストラクタ
	Rigidbody3DComponent(const Rigidbody3DComponent&) = default;
	// ムーブコンストラクタ（移動）
	Rigidbody3DComponent(Rigidbody3DComponent&&) noexcept = default;
	float gravityScale = 1.0f;
	bool isSensor = false; // センサーかどうか
	float friction = 0.3f; // 摩擦係数
	float restitution = 0.0f; // 反発係数
	float mass = 1.0f; // 質量
	bool fixedPositionX = false; // X軸座標固定
	bool fixedPositionY = false; // Y軸座標固定
	bool fixedPositionZ = false; // Z軸座標固定
	bool fixedRotationX = false; // X軸回転固定
	bool fixedRotationY = false; // Y軸回転固定
	bool fixedRotationZ = false; // Z軸回転固定
	physics::d3::Id3BodyType bodyType = physics::d3::Id3BodyType::DYNAMIC; // ボディタイプ
	float3 halfsize = { 0.5f, 0.5f, 0.5f }; // 半径（ボックス形状の場合）
	float3 preHalfsize = { 0.5f,0.5f,0.5f };
	float3 velocity = { 0.0f, 0.0f, 0.0f }; // 速度
	float3 angularVelocity = { 0.0f, 0.0f, 0.0f }; // 角速度
	Quaternion quaternion = { 0.0f, 0.0f, 0.0f, 1.0f }; // 回転
	std::optional<float3> requestedPosition = std::nullopt; // 位置リクエスト
	physics::d3::Id3Body* runtimeBody = nullptr; // 物理エンジンのボディ
	std::optional<Entity> otherEntity = std::nullopt; // 衝突したオブジェクトID
	std::optional<Entity> selfEntity = std::nullopt; // 自分のオブジェクトID
	Rigidbody3DComponent& operator=(const Rigidbody3DComponent& other)
	{
		if (this == &other) return *this;
		friction = other.friction;
		restitution = other.restitution;
		fixedPositionX = other.fixedPositionX;
		fixedPositionY = other.fixedPositionY;
		fixedPositionZ = other.fixedPositionZ;
		fixedRotationX = other.fixedRotationX;
		fixedRotationY = other.fixedRotationY;
		fixedRotationZ = other.fixedRotationZ;
		gravityScale = other.gravityScale;
		halfsize = other.halfsize;
		preHalfsize = other.preHalfsize;
		bodyType = other.bodyType;
		mass = other.mass;
		isSensor = other.isSensor;
		velocity = other.velocity;
		quaternion = other.quaternion;
		return *this;
	}
	// ムーブ代入を明示的に生成
	Rigidbody3DComponent& operator=(Rigidbody3DComponent&&) noexcept = default;
	// 初期化
	void Initialize()
	{
		friction = 0.3f;
		restitution = 0.0f;
		fixedPositionX = false;
		fixedPositionY = false;
		fixedPositionZ = false;
		fixedRotationX = false;
		fixedRotationY = false;
		fixedRotationZ = false;
		gravityScale = 1.0f;
		halfsize = { 0.5f, 0.5f, 0.5f };
		preHalfsize = { 0.5f, 0.5f, 0.5f };
		bodyType = physics::d3::Id3BodyType::DYNAMIC;
		mass = 1.0f;
		isSensor = false;
		velocity = { 0.0f, 0.0f, 0.0f }; // 速度
		quaternion = { 0.0f, 0.0f, 0.0f, 1.0f }; // 回転
		runtimeBody = nullptr; // 物理エンジンのボディ
		otherEntity = std::nullopt;
		selfEntity = std::nullopt;
	}
};

// 3Dボックスコライダー
struct BoxCollider3DComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	BoxCollider3DComponent() = default;
	// コピーコンストラクタ
	BoxCollider3DComponent(const BoxCollider3DComponent&) = default;
	// ムーブコンストラクタ（移動）
	BoxCollider3DComponent(BoxCollider3DComponent&&) noexcept = default;
	float3 halfExtents = { 0.5f, 0.5f, 0.5f }; // 半径
	float density = 1.0f; // 密度
	float friction = 0.3f; // 摩擦係数
	float restitution = 0.0f; // 反発係数
	BoxCollider3DComponent& operator=(const BoxCollider3DComponent& other)
	{
		if (this == &other) return *this;
		halfExtents = other.halfExtents;
		density = other.density;
		friction = other.friction;
		restitution = other.restitution;
		return *this;
	}
	// ムーブ代入を明示的に生成
	BoxCollider3DComponent& operator=(BoxCollider3DComponent&&) noexcept = default;
	// 初期化
	void Initialize()
	{
		halfExtents.Initialize();
		density = 1.0f;
		friction = 0.3f;
		restitution = 0.0f;
	}
};

// マテリアルコンポーネント
struct MaterialComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	MaterialComponent() = default;
	// コピーコンストラクタ
	MaterialComponent(const MaterialComponent&) = default;
	// ムーブコンストラクタ（移動）
	MaterialComponent(MaterialComponent&&) noexcept = default;

	Color color;
	bool enableLighting = true;
	bool enableTexture = false;
	bool uvFlipY = false;
	float4x4 matUV;
	float shininess = 50.0f;
	float cubeUVScale = 1.0f;	// キューブマップのUVスケール
	std::wstring textureName = L"";	// テクスチャ名
	std::optional<uint32_t> textureID = std::nullopt;	// テクスチャID
	std::wstring cubeTextureName = L"";	// キューブマップテクスチャ名
	std::optional<uint32_t> cubeTextureID = std::nullopt;	// キューブマップテクスチャID
	std::optional<uint32_t> mapID = std::nullopt;

	MaterialComponent& operator=(const MaterialComponent& other)
	{
		if (this == &other) return *this;
		color = other.color;
		enableLighting = other.enableLighting;
		enableTexture = other.enableTexture;
		shininess = other.shininess;
		textureName = other.textureName;
		uvFlipY = other.uvFlipY;
		return *this;
	}

	// ムーブ代入を明示的に生成
	MaterialComponent& operator=(MaterialComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		color.Initialize();
		enableLighting = true;
		enableTexture = false;
		uvFlipY = false;
		matUV = float4x4::Identity();
		shininess = 50.0f;
		textureName = L"";
		textureID = std::nullopt;
		mapID = std::nullopt;
	}
};
// エミッターコンポーネント
struct EmitterComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	EmitterComponent() = default;
	// コピーコンストラクタ
	EmitterComponent(const EmitterComponent&) = default;
	// ムーブコンストラクタ（移動）
	EmitterComponent(EmitterComponent&&) noexcept = default;

	RandValue lifeTime;
	PVA position;             // 位置
	PVA rotation;             // 回転
	PVA scale;                // スケール
	float frequency;	// 射出間隔
	float frequencyTime;// 射出間隔調整用時間
	bool emit = false;
	int32_t emitCount = 10; // 射出数
	bool isFadeOut;
	bool isBillboard;	// 4バイト
	std::optional<uint32_t> particleID = std::nullopt;	// パーティクルID
	// バッファインデックス
	std::optional<uint32_t> bufferIndex = std::nullopt;

	std::optional<uint32_t> materalID = std::nullopt;	// マテリアルID

	EmitterComponent& operator=(const EmitterComponent& other)
	{
		if (this == &other) return *this;
		lifeTime = other.lifeTime;
		position = other.position;
		rotation = other.rotation;
		scale = other.scale;
		frequency = other.frequency;
		frequencyTime = other.frequencyTime;
		emit = other.emit;
		emitCount = other.emitCount;
		isFadeOut = other.isFadeOut;
		isBillboard = other.isBillboard;
		return *this;
	}

	// ムーブ代入を明示的に生成
	EmitterComponent& operator=(EmitterComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		lifeTime = {};
		position = {};
		rotation = {};
		scale = {};
		frequency = 0.0f;
		frequencyTime = 0.0f;
		emit = false;
		emitCount = 10;
		isFadeOut = false;
		isBillboard = false;
		particleID = std::nullopt;
		bufferIndex = std::nullopt;
	}
};
// パーティクルコンポーネント
struct ParticleComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	ParticleComponent() = default;
	// コピーコンストラクタ
	ParticleComponent(const ParticleComponent&) = default;
	// ムーブコンストラクタ（移動）
	ParticleComponent(ParticleComponent&&) noexcept = default;

	uint32_t count = 1024;// パーティクル数
	float4x4 matBillboard = float4x4::Identity();
	float time = 0.0f;
	float deltaTime = 0.0f;
	// バッファーインデックス
	std::optional<uint32_t> bufferIndex = std::nullopt;
	std::optional<uint32_t> perFrameBufferIndex = std::nullopt;
	std::optional<uint32_t> freeListIndexBufferIndex = std::nullopt;
	std::optional<uint32_t> freeListBufferIndex = std::nullopt;

	ParticleComponent& operator=(const ParticleComponent& other)
	{
		if (this == &other) return *this;
		count = other.count;
		time = other.time;
		deltaTime = other.deltaTime;
		return *this;
	}

	// ムーブ代入を明示的に生成
	ParticleComponent& operator=(ParticleComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		count = 1024;
		matBillboard = float4x4::Identity();
		time = 0.0f;
		deltaTime = 0.0f;
		bufferIndex = std::nullopt;
		perFrameBufferIndex = std::nullopt;
		freeListIndexBufferIndex = std::nullopt;
		freeListBufferIndex = std::nullopt;
	}
};
// エフェクトコンポーネント
struct EffectComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	EffectComponent() = default;
	// コピーコンストラクタ
	EffectComponent(const EffectComponent&) = default;
	// ムーブコンストラクタ（移動）
	EffectComponent(EffectComponent&&) noexcept = default;
	// コピー演算子
	EffectComponent& operator=(const EffectComponent& other)
	{
		if (this == &other) return *this;
		effectName = other.effectName;
		//isRun = other.isRun;
		//isLoop = other.isLoop;
		//root = other.root;
		return *this;
	}
	// ムーブ代入を明示的に生成
	EffectComponent& operator=(EffectComponent&&) noexcept = default;
	std::wstring effectName = L"";	// エフェクト名
	bool isRun = false;	// 実行フラグ
	bool isLoop = true;	// ループフラグ
	std::pair<uint32_t, EffectRootData> root;
};
// スプライトコンポーネント
struct UISpriteComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	UISpriteComponent() = default;
	// コピーコンストラクタ
	UISpriteComponent(const UISpriteComponent&) = default;
	// ムーブコンストラクタ（移動）
	UISpriteComponent(UISpriteComponent&&) noexcept = default;

	float2 position{ 0.0f,0.0f };// 位置
	float rotation = 0.0f;// Z軸回転
	float2 scale = { 1.0f,1.0f };// スケール
	float4x4 matWorld = float4x4::Identity();
	float2 uvPos = { 0.0f,0.0f };
	float uvRot = 0.0f;
	float2 uvScale = { 1.0f,1.0f };
	float2 anchorPoint = { 0.0f,0.0f };// アンカーポイント
	float2 size = { 64.0f,64.0f };// サイズ
	float2 textureLeftTop = { 0.0f,0.0f };// テクスチャの左上座標
	float2 textureSize = { 64.0f,64.0f };// テクスチャのサイズ

	std::optional<uint32_t> mapID = std::nullopt;

	UISpriteComponent& operator=(const UISpriteComponent& other)
	{
		if (this == &other) return *this;
		position = other.position;
		rotation = other.rotation;
		scale = other.scale;
		//uvPos = other.uvPos;
		//uvRot = other.uvRot;
		//uvScale = other.uvScale;
		anchorPoint = other.anchorPoint;
		size = other.size;
		textureLeftTop = other.textureLeftTop;
		textureSize = other.textureSize;
		return *this;
	}

	// ムーブ代入を明示的に生成
	UISpriteComponent& operator=(UISpriteComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		position.Initialize();
		rotation = 0.0f;
		scale.Initialize();
		matWorld = float4x4::Identity();
		uvPos.Initialize();
		uvRot = 0.0f;
		uvScale.Initialize();
		anchorPoint.Initialize();
		size.Initialize();
		textureLeftTop.Initialize();
		textureSize.Initialize();
		mapID = std::nullopt;
	}
};

// ライトコンポーネント
struct LightComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	LightComponent() = default;
	// コピーコンストラクタ
	LightComponent(const LightComponent&) = default;
	// ムーブコンストラクタ（移動）
	LightComponent(LightComponent&&) noexcept = default;

	Color color;		// 色
	float intensity=1.0f;	// 強度
	float range=10.0f;		// 適用距離
	float decay=1.0f;		// 減衰率
	float spotAngle = 45.0f;	// スポットライトの角度
	float spotFalloffStart = 0.0f;
	LightType type = LightType::Directional;		// ライトの種類
	bool active = true;	// ライトの有効無効
	std::optional<uint32_t> mapID = std::nullopt;	// マップインデックス

	LightComponent& operator=(const LightComponent& other)
	{
		if (this == &other) return *this;
		color = other.color;
		intensity = other.intensity;
		range = other.range;
		decay = other.decay;
		spotAngle = other.spotAngle;
		type = other.type;
		active = other.active;
		return *this;
	}

	// ムーブ代入を明示的に生成
	LightComponent& operator=(LightComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		color.Initialize();
		intensity = 1.0f;
		range = 10.0f;
		decay = 1.0f;
		spotAngle = 45.0f;
		spotFalloffStart = 0.0f;
		type = LightType::Directional;
		active = true;
	}
};

// Audioコンポーネント
struct SoundData;
struct AudioComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	AudioComponent() = default;
	// コピーコンストラクタ
	AudioComponent(const AudioComponent&) = default;
	// ムーブコンストラクタ（移動）
	AudioComponent(AudioComponent&&) noexcept = default;

	//std::string audioName = "";	// オーディオ名
	//std::optional<uint32_t> audioID = std::nullopt;	// オーディオID
	std::vector<SoundData> soundData;	// サウンドデータ
	bool isLoop = false;	// ループフラグ
	bool isPlay = false;	// 再生フラグ
	bool isPause = false;	// 一時停止フラグ
	bool isStop = false;	// 停止フラグ
	//bool is3D = false;	// 3Dフラグ
	AudioComponent& operator=(const AudioComponent& other)
	{
		if (this == &other) return *this;
		//audioName = other.audioName;
		isLoop = other.isLoop;
		isPlay = other.isPlay;
		isPause = other.isPause;
		isStop = other.isStop;
		return *this;
	}

	// ムーブ代入を明示的に生成
	AudioComponent& operator=(AudioComponent&&) noexcept = default;

	// 初期化
	void Initialize()
	{
		//audioName = "";
		//audioID = std::nullopt;
		isLoop = false;
		isPlay = false;
		isPause = false;
		isStop = false;
	}
};

// アニメーションコンポーネント
struct AnimationComponent : public IComponentTag
{
	// デフォルト ctor はそのまま
	AnimationComponent() = default;
	// コピーコンストラクタ
	AnimationComponent(const AnimationComponent& other)
	{
		transitionDuration = other.transitionDuration;
		numAnimation = other.numAnimation;
		animationIndex = other.animationIndex;
		modelName = other.modelName;
	}
	// ムーブコンストラクタ（移動）
	AnimationComponent(AnimationComponent&&) noexcept = default;

	// コピー演算子
	AnimationComponent& operator=(const AnimationComponent& other)
	{
		if (this == &other) return *this;
		transitionDuration = other.transitionDuration;
		numAnimation = other.numAnimation;
		animationIndex = other.animationIndex;
		modelName = other.modelName;
		return *this;
	}

	// ムーブ代入を明示的に生成
	AnimationComponent& operator=(AnimationComponent&&) noexcept = default;

	float time = 0.0f;// 現在のアニメーションの時間
	float transitionTime = 0.0f;// 遷移中経過時間
	float transitionDuration = 1.0f;// 遷移にかける時間
	float lerpTime = 0.0f;// 線形補間の時間
	bool transition = false;
	bool isEnd = false;// ループするかどうか
	bool isRun = true;// アニメーションを再生するかどうか
	bool isRestart = true;// アニメーションを再生するかどうか
	uint32_t numAnimation = 0;// アニメーションの数
	uint32_t animationIndex = 0;// アニメーションのIndex
	uint32_t prevAnimationIndex = 0;// 1つ前のアニメーションのIndex
	uint32_t transitionIndex = 0;// 遷移スタートのアニメーションのIndex
	//uint32_t nowFrame = 0;// 現在のフレーム
	//uint32_t allFrame = 0;// 全フレーム数
	std::wstring modelName = L"";	// モデル名
	std::optional<uint32_t> boneOffsetID = std::nullopt;	// ボーンオフセットID

	std::optional<Skeleton> skeleton = std::nullopt;	// スケルトンデータ
	std::vector<SkinCluster> skinClusters;	// スキンクラスター

	// 初期化
	void Initialize()
	{
		time = 0.0f;
		transitionTime = 0.0f;
		transitionDuration = 1.0f;
		lerpTime = 0.0f;
		transition = false;
		isEnd = false;
		isRun = true;
		isRestart = true;
		numAnimation = 0;
		animationIndex = 0;
		prevAnimationIndex = 0;
		transitionIndex = 0;
		modelName = L"";
		boneOffsetID = std::nullopt;
		skeleton = std::nullopt;
		skinClusters.clear();
	}
	
};

// 全種類のコンポーネントをまとめた構造体
struct ComponentBlock
{
	TransformComponent transform;			// 変形コンポーネント
	CameraComponent camera;				// カメラコンポーネント
	MeshFilterComponent meshFilter;		// メッシュフィルターコンポーネント
	MeshRendererComponent meshRenderer;	// メッシュレンダラーコンポーネント
	ScriptComponent script;				// スクリプトコンポーネント
	std::vector<LineRendererComponent> lineRenderers;	// ラインレンダラーコンポーネント
	Rigidbody2DComponent rigidbody2D;	// 2D物理挙動コンポーネント
	BoxCollider2DComponent boxCollider2D; // 2D矩形コライダー
	MaterialComponent material;			// マテリアルコンポーネント
	EmitterComponent emitter;				// エミッターコンポーネント
	ParticleComponent particle;			// パーティクルコンポーネント
	EffectComponent effect;				// エフェクトコンポーネント
	UISpriteComponent uiSprite;			// UIスプライトコンポーネント
	LightComponent light;					// ライトコンポーネント
	AudioComponent audio;					// オーディオコンポーネント
	AnimationComponent animation;			// アニメーションコンポーネント
};

// マルチコンポーネントを許可
template<>
struct IsMultiComponent<LineRendererComponent> : std::true_type {};

// オブジェクトタイプによるコンポーネントの所持許可
// デフォルト : すべてfalse
template<ObjectType T,typename Component>
constexpr bool IsComponentAllowed = false;

// MeshObject
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, TransformComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, MeshFilterComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, MeshRendererComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, ScriptComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, MaterialComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, LineRendererComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, Rigidbody2DComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, BoxCollider2DComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, Rigidbody3DComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, AudioComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::MeshObject, AnimationComponent> = true;

// Camera
template<> constexpr bool IsComponentAllowed<ObjectType::Camera, TransformComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::Camera, CameraComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::Camera, ScriptComponent> = true;

// ParticleSystem
template<> constexpr bool IsComponentAllowed<ObjectType::ParticleSystem, TransformComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::ParticleSystem, MeshFilterComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::ParticleSystem, MeshRendererComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::ParticleSystem, MaterialComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::ParticleSystem, EmitterComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::ParticleSystem, ParticleComponent> = true;

// Effect
template<> constexpr bool IsComponentAllowed<ObjectType::Effect, TransformComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::Effect, EffectComponent> = true;

// UI
template<> constexpr bool IsComponentAllowed<ObjectType::UI, TransformComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::UI, UISpriteComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::UI, ScriptComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::UI, MaterialComponent> = true;

// Light
template<> constexpr bool IsComponentAllowed<ObjectType::Light, TransformComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::Light, LightComponent> = true;
template<> constexpr bool IsComponentAllowed<ObjectType::Light, ScriptComponent> = true;

// ランタイム対応
template<typename Component>
bool IsComponentAllowedAtRuntime(ObjectType type)
{
	switch (type)
	{
	case ObjectType::MeshObject:
		return IsComponentAllowed<ObjectType::MeshObject, Component>;
	case ObjectType::Camera:
		return IsComponentAllowed<ObjectType::Camera, Component>;
	case ObjectType::ParticleSystem:
		return IsComponentAllowed<ObjectType::ParticleSystem, Component>;
	case ObjectType::Effect:
		return IsComponentAllowed<ObjectType::Effect, Component>;
	case ObjectType::UI:
		return IsComponentAllowed<ObjectType::UI, Component>;
	case ObjectType::Light:
		return IsComponentAllowed<ObjectType::Light, Component>;
	case ObjectType::Count:
		return false;
	default:
		return false;
	}
}
