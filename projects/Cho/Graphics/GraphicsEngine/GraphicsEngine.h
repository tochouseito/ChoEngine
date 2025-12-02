#pragma once
#include "Engine/Engine.h"
#include "Graphics/GraphicsCore/GraphicsCore.h"
#include "Graphics/DepthManager/DepthManager.h"
#include "Graphics/PipelineManager/PipelineManager.h"
#include "SDK/DirectX/DirectX12/SwapChain/SwapChain.h"
#include "SDK/DirectX/DirectX12/DescriptorHeap/DescriptorHeap.h"
#include "SDK/DirectX/DirectX12/GpuBuffer/GpuBuffer.h"
#include "SDK/DirectX/DirectX12/ColorBuffer/ColorBuffer.h"
#include <array>

enum GameRenderTextureType
{
	GameScreenTexture = 0,			// ゲーム画面用描画テクスチャ
	GameGBufferTexture,				// ゲームのGバッファ用描画テクスチャ
	GameLightingTexture,			// ゲームのライティング用描画テクスチャ
	GameForwardTexture,				// ゲームのフォワード描画用描画テクスチャ
	GamePostProcessTexture,			// ゲームのポストプロセス用描画テクスチャ
	GameRenderTextureTypeCount,		// 種類数(使用禁止)
};

// Editor時のみ使用
enum SceneRenderTextureType
{
	SceneScreenTexture = 0,			// シーン画面用描画テクスチャ
	SceneGBufferTexture,			// シーンのGバッファ用描画テクスチャ
	SceneLightingTexture,			// シーンのライティング用描画テクスチャ
	SceneForwardTexture,			// シーンのフォワード描画用テクスチャ
	ScenePostProcessTexture,		// シーンのポストプロセス用描画テクスチャ
	EffectEditorTexture,			// エフェクトエディタ用描画テクスチャ
	DebugDrawTexture,				// デバッグ用描画テクスチャ
	SceneRenderTextureTypeCount,	// 種類数(使用禁止)
};

enum DrawPass
{
	GBuffers = 0,
	Lighting,
	Forward,
	PostProcess,
	SwapChainPass,
	EditorView,
	PassCount,
};

enum RenderMode
{
	Game=0,
	Scene,
	Release,
	RenderModeCount,
};

enum ViewportType
{
	ViewportGame = 0,
	ViewportSwapChain,
};

struct RenderTexture
{
	std::optional<uint32_t> m_BufferIndex = std::nullopt;
	UINT64 m_Width = 0;
	UINT m_Height = 0;
};

class ResourceManager;
class ImGuiManager;
class GameCore;
class EngineCommand;
class ThreadManager;
class GraphicsEngine : public Engine
{
	friend class SwapChain;
	friend class TextureManager;
	friend class PipelineManager;
	friend class ResourceManager;
	friend class EngineCommand;
	friend class ParticleEmitterSystem;
	friend class EffectEditorSystem;
public:
	// Constructor
    GraphicsEngine(ID3D12Device8* device, ResourceManager* resourceManager, RuntimeMode mode, ThreadManager* threadManager) :
        m_Device(device), m_ResourceManager(resourceManager), Engine(mode), m_ThreadManager(threadManager)
    {
        m_GraphicsCore = std::make_unique<GraphicsCore>(device, threadManager);
        m_DepthManager = std::make_unique<DepthManager>();
        m_PipelineManager = std::make_unique<PipelineManager>(resourceManager,this);
    }
	// Destructor
	~GraphicsEngine() = default;
	void Finalize() override
	{
		// GPUの完了待ち
		WaitForGPU(Graphics);
		WaitForGPU(Compute);
		WaitForGPU(Copy);
	}
	void Init();
	void SetEngineCommand(EngineCommand* engineCommand) { m_EngineCommand = engineCommand; }
	// SwapChainの生成
	void CreateSwapChain(IDXGIFactory7* dxgiFactory);
	void PreRender();
	void Render(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode = RenderMode::Game);
	void PostRender(ImGuiManager* imgui, RenderMode mode);
	// レンダーターゲットテクスチャ、ImGui、スワップチェーンのリサイズ
	void ScreenResize();
	// 解像度の取得
	UINT64 GetResolutionWidth() { return m_ResolutionWidth; }
	UINT GetResolutionHeight() { return m_ResolutionHeight; }

	PipelineManager* GetPipelineManager() { return m_PipelineManager.get(); }

	// GameTextureのBufferIDを取得
	uint32_t GetGameTextureBufferID() { return m_GameRenderTextures[GameScreenTexture].m_BufferIndex.value(); }
	// SceneTextureのBufferIDを取得
	uint32_t GetSceneTextureBufferID() { return m_SceneRenderTextures[SceneScreenTexture].m_BufferIndex.value(); }
	// EffectEditTextureのBufferIDを取得
	uint32_t GetEffectEditTextureBufferID() { return m_SceneRenderTextures[EffectEditorTexture].m_BufferIndex.value(); }
private:
	// コマンドコンテキストの取得
	CommandContext* GetCommandContext() { return m_GraphicsCore->GetCommandManager()->GetCommandContext(); }
	// コマンドマネージャー取得
	CommandManager* GetCommandManager() { return m_GraphicsCore->GetCommandManager(); }
	// 記録開始
	void BeginCommandContext(CommandContext* context);
	// 記録終了
	void EndCommandContext(CommandContext* context,const QueueType& queue);
	// GPU待機
	void WaitForGPU(const QueueType& queue);
	// レンダーターゲットの設定
	void SetRenderTargets(CommandContext* context, DrawPass pass, RenderMode mode,bool isSetTarget);
	// 描画設定コマンド
	void SetRenderState(CommandContext* context,ViewportType type);
	void DrawGBuffers(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void DrawLighting(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void DrawForward(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void DrawPostProcess(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void DrawEditorView(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	// 深度バッファの生成
	void CreateDepthBuffer();
	// オフスクリーンバッファの生成
	void CreateOffscreenBuffer();
	// デバッグ用深度バッファの生成
	void CreateDebugDepthBuffer();

	// タイプごとに描画
	void DrawParticles(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void EffectEditorDraw(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void DrawUI(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);
	void SkyboxRender(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode);

	ID3D12Device8* m_Device = nullptr;
	ResourceManager* m_ResourceManager = nullptr;
	EngineCommand* m_EngineCommand = nullptr;
    ThreadManager* m_ThreadManager = nullptr;

	std::unique_ptr<SwapChain> m_SwapChain = nullptr;
	std::unique_ptr<GraphicsCore> m_GraphicsCore = nullptr;
	std::unique_ptr<DepthManager> m_DepthManager = nullptr;
	std::unique_ptr<PipelineManager> m_PipelineManager = nullptr;

	std::array<RenderTexture, GameRenderTextureType::GameRenderTextureTypeCount> m_GameRenderTextures;
	std::array<RenderTexture, SceneRenderTextureType::SceneRenderTextureTypeCount> m_SceneRenderTextures;

	UINT64 m_ResolutionWidth = 1920;
	UINT m_ResolutionHeight = 1080;
};

