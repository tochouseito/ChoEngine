#include "pch.h"
#include "EngineCommand.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "GameCore/GameCore.h"
#include "Platform/FileSystem/FileSystem.h"
#include "Core/Utility/EffectStruct.h"
#include "EngineCommand/EngineCommands.h"

EngineCommand::EngineCommand(GameCore* gameCore, ResourceManager* resourceManager, GraphicsEngine* graphicsEngine,InputManager* input, PlatformLayer* platformLayer, theatria::Config* config):
	m_GameCore(gameCore),
	m_ResourceManager(resourceManager),
	m_GraphicsEngine(graphicsEngine),
	m_InputManager(input),
	m_PlatformLayer(platformLayer),
    m_Config(config)
{
	
}

D3D12_GPU_DESCRIPTOR_HANDLE EngineCommand::GetGameTextureHandle()
{
	// ゲームレンダリングテクスチャのバッファインデックスを取得
	uint32_t bufferIndex = m_GraphicsEngine->GetGameTextureBufferID();
	// ハンドルを取得
	return m_ResourceManager->GetBuffer<ColorBuffer>(bufferIndex)->GetSRVGpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE EngineCommand::GetSceneTextureHandle()
{
	// シーンレンダリングテクスチャのバッファインデックスを取得
	uint32_t bufferIndex = m_GraphicsEngine->GetSceneTextureBufferID();
	// ハンドルを取得
	return m_ResourceManager->GetBuffer<ColorBuffer>(bufferIndex)->GetSRVGpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE EngineCommand::GetEffectEditTextureHandle()
{
	// シーンレンダリングテクスチャのバッファインデックスを取得
	uint32_t bufferIndex = m_GraphicsEngine->GetEffectEditTextureBufferID();
	// ハンドルを取得
	return m_ResourceManager->GetBuffer<ColorBuffer>(bufferIndex)->GetSRVGpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE EngineCommand::GetTextureHandle(const std::wstring& name)
{
	return m_ResourceManager->GetBuffer<PixelBuffer>(m_ResourceManager->GetTextureManager()->GetTextureData(name)->bufferIndex)->GetSRVGpuHandle();
}

void EngineCommand::SaveProjectFile()
{
	theatria::FileSystem::SaveProject(m_EditorManager,m_GameCore->GetSceneManager(), m_GameCore->GetGameWorld(), m_GameCore->GetECSManager(),this);
}

void EngineCommand::GenerateScript(const std::string& scriptName)
{
    FileSystem::ScriptProject::AddClassFileToProject(ConvertString(scriptName));
	// プロジェクトの保存処理
	//theatria::FileSystem::ScriptProject::SaveAndBuildSolution(false);
	// スクリプトファイルの生成
	//theatria::FileSystem::ScriptProject::GenerateScriptFiles(scriptName);
	// プロジェクトファイルを更新
	//theatria::FileSystem::ScriptProject::UpdateVcxproj();
	// スクリプトコンテナに追加
	m_ResourceManager->GetScriptContainer()->AddScriptData(scriptName);
    // プロジェクトの保存処理
    //theatria::FileSystem::ScriptProject::SaveAndBuildSolution(false);
}

void EngineCommand::GameRun(const bool& isDebugger)
{
	m_GameCore->GameRun(isDebugger);
}

void EngineCommand::GameStop()
{
	m_GameCore->GameStop();
}

bool EngineCommand::IsGameRunning() { return m_GameCore->IsRunning(); }

UINT64 EngineCommand::GetGameResolutionX() const
{
	return m_GraphicsEngine->m_ResolutionWidth;
}

UINT EngineCommand::GetGameResolutionY() const
{
	return m_GraphicsEngine->m_ResolutionHeight;
}
