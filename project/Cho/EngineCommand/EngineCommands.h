#pragma once
#include <optional>
#include "EngineCommand/EngineCommand.h"
#include "Core/Utility/Components.h"
class GameObject;
class IPrefab;
class CPrefab;
// 3Dオブジェクトを追加するコマンド
class Add3DObjectCommand :public IEngineCommand
{
public:
	Add3DObjectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
	
	// オブジェクトのハンドルを取得
	ObjectHandle GetObjectHandle() const { return m_Handle; }
private:
	ObjectHandle m_Handle;
};
// カメラオブジェクトを追加するコマンド
class AddCameraObjectCommand :public IEngineCommand
{
public:
	AddCameraObjectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
	// オブジェクトのハンドルを取得
	ObjectHandle GetObjectHandle() const { return m_Handle; }
private:
	ObjectHandle m_Handle;
};
// パーティクルシステムオブジェクトを追加するコマンド
class AddParticleSystemObjectCommand :public IEngineCommand
{
public:
	AddParticleSystemObjectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	ObjectHandle m_Handle;
};
// Effectオブジェクトを追加するコマンド
class AddEffectObjectCommand :public IEngineCommand
{
public:
	AddEffectObjectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	ObjectHandle m_Handle;
};
// UIオブジェクトを追加するコマンド
class AddUIObjectCommand :public IEngineCommand
{
public:
	AddUIObjectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	ObjectHandle m_Handle;
};
// LightObjectを追加するコマンド
class AddLightObjectCommand :public IEngineCommand
{
public:
	AddLightObjectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	ObjectHandle m_Handle;
};
// MeshFilterComponentを追加するコマンド
class AddMeshFilterComponent :public IEngineCommand
{
public:
	AddMeshFilterComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// MeshRendererComponentを追加するコマンド
class AddMeshRendererComponent :public IEngineCommand
{
public:
	AddMeshRendererComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// MainCameraを設定するコマンド
class SetMainCamera :public IEngineCommand
{
public:
	SetMainCamera(const ObjectHandle& setCameraHandle):
		m_SetCameraHandle(setCameraHandle)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	ObjectHandle m_PreCameraHandle{};
	ObjectHandle m_SetCameraHandle{};
};
// スクリプトコンポーネントを追加するコマンド
class AddScriptComponent :public IEngineCommand
{
public:
	AddScriptComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// ラインレンダラーコンポーネントを追加するコマンド
class AddLineRendererComponent :public IEngineCommand
{
public:
	AddLineRendererComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
	uint32_t m_MapID;
};
// Rigidbody2Dコンポーネントを追加するコマンド
class AddRigidbody2DComponent :public IEngineCommand
{
public:
	AddRigidbody2DComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// BoxCollider2Dコンポーネントを追加するコマンド
class AddBoxCollider2DComponent :public IEngineCommand
{
public:
	AddBoxCollider2DComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// Rigidbody3Dコンポーネントを追加するコマンド
class AddRigidbody3DComponent :public IEngineCommand
{
	public:
	AddRigidbody3DComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// オブジェクトを削除するコマンド
class DeleteObjectCommand : public IEngineCommand
{
public:
	DeleteObjectCommand(const ObjectHandle& handle) :
		m_Handle(handle)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	std::unique_ptr<CPrefab> m_Prefab; // 削除前のPrefab情報を保持
	ObjectHandle m_Handle{};
};
// オブジェクトの名前を変更するコマンド
class RenameObjectCommand : public IEngineCommand
{
public:
	RenameObjectCommand(const ObjectHandle& handle, const std::wstring& name) :
		m_Handle(handle), m_NewName(name)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	ObjectHandle m_Handle;
	std::wstring m_NewName;
	std::wstring m_PreName;
};
// マテリアルコンポーネントを追加するコマンド
class AddMaterialComponent :public IEngineCommand
{
public:
	AddMaterialComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// Emitterコンポーネントを追加するコマンド
class AddEmitterComponent :public IEngineCommand
{
public:
	AddEmitterComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// Particleコンポーネントを追加するコマンド
class AddParticleComponent :public IEngineCommand
{
public:
	AddParticleComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// 物理エンジンの重力を設定するコマンド
class SetGravityCommand : public IEngineCommand
{
public:
	SetGravityCommand(const float3& gravity) :
		m_Gravity(gravity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	float3 m_Gravity;
	float3 m_PreGravity;
};
// Audioコンポーネント追加
class AddAudioComponent :public IEngineCommand
{
public:
	AddAudioComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};
// AnimationComponent追加
class AddAnimationComponent :public IEngineCommand
{
public:
	AddAnimationComponent(const uint32_t& entity) :
		m_Entity(entity)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};

// Effectを新規作成するコマンド
class CreateEffectCommand : public IEngineCommand
{
	public:
	CreateEffectCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};

// EffectNodeを追加するコマンド
class AddEffectNodeCommand : public IEngineCommand
{
	public:
	AddEffectNodeCommand()
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	uint32_t m_Entity;
};

// Objectを複製するコマンド
class CloneObjectCommand : public IEngineCommand
{
public:
	CloneObjectCommand(const ObjectHandle& handle):
		m_Src(handle)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
	// オブジェクトのIDを取得
	ObjectHandle GetDstHandle() const { return m_Dst; }
private:
	ObjectHandle m_Src;
	ObjectHandle m_Dst;
};

// Objectをコピーするコマンド
class CopyObjectCommand : public IEngineCommand
{
	public:
	CopyObjectCommand(const ObjectHandle& handle) :
		m_Src(handle)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
	// コピー先のオブジェクトのハンドルを取得
	ObjectHandle GetDstHandle() const { return m_Dst; }
private:
	ObjectHandle m_Src; // コピー元のオブジェクトのハンドル
	ObjectHandle m_Dst; // コピー先のオブジェクトのハンドル
};

// EditorのSceneを変えるコマンド
class ChangeEditorSceneCommand : public IEngineCommand
{
	public:
	ChangeEditorSceneCommand(const std::wstring& sceneName) :
		m_SceneName(sceneName)
	{
	}
	bool Execute(EngineCommand* edit)override;
	bool Undo(EngineCommand* edit)override;
private:
	std::wstring m_SceneName;
	std::wstring m_PreSceneName; // 前のシーン名を保持
};
