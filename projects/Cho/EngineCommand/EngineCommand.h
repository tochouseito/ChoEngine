#pragma once
#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"
#include "Resources/IntegrationData/IntegrationData.h"
class GameCore;
class ResourceManager;
class GraphicsEngine;
class GameObject;
class InputManager;
class EngineCommand;
class PlatformLayer;
namespace theatria
{
    class Config;
};
// インタフェース
class IEngineCommand
{
public:
	// Constructor
	IEngineCommand() = default;
	// Destructor
	virtual ~IEngineCommand() = default;
	virtual bool Execute(EngineCommand* edit) = 0;
	virtual bool Undo(EngineCommand* edit) = 0;
};
class EngineCommand
{
	friend class EditorManager;

	// Command クラス
	friend class Add3DObjectCommand;
	friend class AddCameraObjectCommand;
	friend class AddMeshFilterComponent;
	friend class AddMeshRendererComponent;
	friend class SetMainCamera;
	friend class AddScriptComponent;
	friend class AddLineRendererComponent;
	friend class AddRigidbody2DComponent;
	friend class AddBoxCollider2DComponent;
	friend class DeleteObjectCommand;
	friend class RenameObjectCommand;
	friend class AddMaterialComponent;
	friend class AddParticleSystemObjectCommand;
	friend class AddEmitterComponent;
	friend class AddParticleComponent;
	friend class AddEffectObjectCommand;
	friend class AddUIObjectCommand;
	friend class SetGravityCommand;
	friend class AddLightObjectCommand;
	friend class AddAudioComponent;
	friend class AddAnimationComponent;
	friend class CopyGameObjectCommand;
	friend class CreateEffectCommand;
	friend class AddEffectNodeCommand;
	friend class CloneObjectCommand;
	friend class AddRigidbody3DComponent;
	// Editor
	friend class EffectEditorSystem;
	friend class EffectEditor;

	friend class GraphicsEngine;
public:
	// Constructor
	EngineCommand(GameCore* gameCore, ResourceManager* resourceManager, GraphicsEngine* graphicsEngine,InputManager* input, PlatformLayer* platformLayer, theatria::Config* config);
	// Destructor
	~EngineCommand()
	{
	}
	// コマンド実行
	bool ExecuteCommand(std::unique_ptr<IEngineCommand> command)
	{
		if (command->Execute(this))
		{
			m_Commands.push_back(std::move(command));
			return true;
		}
		return false;
	}
	// コマンド取り消し
	bool UndoCommand()
	{
		if (!m_Commands.empty())
		{
			if (m_Commands.back()->Undo(this))
			{
				m_Commands.pop_back();
				return true;
			}
			// 取り消しに失敗した場合は、コマンドを削除しない
			return false;
		}
		return false;
	}
	// コマンドクリア
	void ClearCommands()
	{
		m_Commands.clear();
	}
	// ゲームコアを取得
	GameCore* GetGameCore() { return m_GameCore; }
	// リソースマネージャを取得
	ResourceManager* GetResourceManager() { return m_ResourceManager; }
	// グラフィックスエンジンを取得
	GraphicsEngine* GetGraphicsEngine() { return m_GraphicsEngine; }
	// エディタマネージャを取得
	EditorManager* GetEditorManager() { return m_EditorManager; }
	// 入力マネージャを取得
	InputManager* GetInputManager() { return m_InputManager; }
	// プラットフォームレイヤーを取得
	PlatformLayer* GetPlatformLayer() { return m_PlatformLayer; }
    // Configを取得
    theatria::Config* GetConfig() { return m_Config; }
	// レンダリングテクスチャのハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetGameTextureHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetSceneTextureHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetEffectEditTextureHandle();
	// テクスチャを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(const std::wstring& name);
	// プロジェクトの保存
	void SaveProjectFile();
	// Test:スクリプト作成	
	void GenerateScript(const std::string& scriptName);
	// ゲーム再生
	void GameRun(const bool& isDebugger);
	// ゲーム停止
	void GameStop();
	// ゲーム実行中か
	bool IsGameRunning();
	// ゲームの解像度を取得
	UINT64 GetGameResolutionX() const;
	UINT GetGameResolutionY() const;
private:
	void SetEditorManager(EditorManager* editorManager) { m_EditorManager = editorManager; }

	EditorManager* m_EditorManager = nullptr;

	GameCore* m_GameCore = nullptr;
	ResourceManager* m_ResourceManager = nullptr;
	GraphicsEngine* m_GraphicsEngine = nullptr;
	InputManager* m_InputManager = nullptr;
	PlatformLayer* m_PlatformLayer = nullptr;
    theatria::Config* m_Config = nullptr;
	std::vector<std::unique_ptr<IEngineCommand>> m_Commands;
};

