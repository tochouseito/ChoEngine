#include "pch.h"
#include "GraphicsEngine.h"
#include "OS/Windows/WinApp/WinApp.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "EngineCommand/EngineCommand.h"
#include "SDK/ImGui/ImGuiManager/ImGuiManager.h"
#include "GameCore/GameCore.h"
#include "Core/ChoLog/ChoLog.h"
#include "SDK/ImGui/ImGuiUtlity/ImGuiUtlity.h"
using namespace theatria;

void GraphicsEngine::Init()
{
	// DepthBufferの生成
	CreateDepthBuffer();
	// オフスクリーンレンダリング用のリソースを作成
	CreateOffscreenBuffer();
	// デバッグ用のリソースを作成
	CreateDebugDepthBuffer();
	//CreateDebugOffscreenBuffer();
	// パイプラインの生成
	m_PipelineManager->Initialize(m_Device);
}

void GraphicsEngine::CreateSwapChain(IDXGIFactory7* dxgiFactory)
{
	// SwapChainの生成
	m_SwapChain = std::make_unique<SwapChain>(
		dxgiFactory,
		this,
		m_GraphicsCore->GetCommandQueue(QueueType::Graphics),
		m_GraphicsCore->GetCommandManager(),
		WinApp::GetHWND(),
		WinApp::GetWindowWidth(),
		WinApp::GetWindowHeight()
	);
	// SwapChainのリソースでRTVを作成
	m_SwapChain->CreateResource(m_Device, m_ResourceManager);
}

void GraphicsEngine::PreRender()
{
	//// コマンドマネージャー
	//CommandManager* commandManager = m_GraphicsCore->GetCommandManager();
	//// コマンドリストの取得
	//CommandContext* context = commandManager->GetCommandContext();
	//BeginCommandContext(context);
	//EndCommandContext(context,Graphics);
	//// GPUの完了待ち
	//WaitForGPU(Graphics);
}

void GraphicsEngine::Render(ResourceManager& resourceManager, GameCore& gameCore,RenderMode mode)
{
	// ディファードレンダリング
	DrawGBuffers(resourceManager, gameCore, mode);
	// ライティング
	DrawLighting(resourceManager, gameCore, mode);
	// フォワードレンダリング
	DrawForward(resourceManager, gameCore, mode);
	// ポストプロセス
	DrawPostProcess(resourceManager, gameCore, mode);
	// エディタビュー
	DrawEditorView(resourceManager, gameCore, mode);
}

void GraphicsEngine::PostRender(ImGuiManager* imgui, RenderMode mode)
{
	// コマンドマネージャー
	CommandManager* commandManager = m_GraphicsCore->GetCommandManager();
	// コマンドリストの取得
	CommandContext* context = commandManager->GetCommandContext();
	BeginCommandContext(context);
	// RTVの設定
	SetRenderTargets(context, DrawPass::SwapChainPass, mode, true);
	// 描画設定
	SetRenderState(context, ViewportSwapChain);
	if(mode == RenderMode::Release)
	{
		// パイプラインセット
		context->SetGraphicsPipelineState(m_PipelineManager->GetScreenCopyPSO().pso.Get());
		// ルートシグネチャセット
		context->SetGraphicsRootSignature(m_PipelineManager->GetScreenCopyPSO().rootSignature.Get());
		// PostProcessRenderTextureをセット
		ColorBuffer* setTex = nullptr;
		setTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GamePostProcessTexture].m_BufferIndex);
		// ポストプロセスレンダリングテクスチャをセット
		context->SetGraphicsRootDescriptorTable(0, setTex, ViewType::ShaderResourceView);
		// DrawCall
		context->DrawInstanced(3, 1, 0, 0);
	}
	else
	{
		// ImGuiの描画
		imgui->Draw(context->GetCommandList());
	}
	// RenderTargetsの状態を戻す
	SetRenderTargets(context, DrawPass::SwapChainPass, mode, false);
	// コマンドリストのクローズ
	context->Close();
	// コマンドリストの実行
	m_GraphicsCore->GetCommandManager()->ExecuteCommandList(context->GetCommandList(), Graphics);
	// SwapChainのPresent
	m_SwapChain->Present();
	// シグナル
	m_GraphicsCore->GetCommandManager()->Signal(Graphics);
	// コマンドリストの返却
	m_GraphicsCore->GetCommandManager()->ReturnCommandContext(context);
	// GPUの完了待ち
	WaitForGPU(Graphics);
}

void GraphicsEngine::ScreenResize()
{
	// GPUの完了待ち
	WaitForGPU(Graphics);
	WaitForGPU(Compute);
	WaitForGPU(Copy);
	// ImGuiのリサイズ
	ImGuiUtility::DisplayResize(static_cast<float>(WinApp::GetWindowWidth()), static_cast<float>(WinApp::GetWindowHeight()));
	// SwapChainのリサイズ
	m_SwapChain->Resize(m_Device, WinApp::GetWindowWidth(), WinApp::GetWindowHeight());
	// DepthBufferのリサイズ
	m_DepthManager->ResizeDepthBuffer(m_Device, m_ResourceManager, m_ResolutionWidth, m_ResolutionHeight);
	// オフスクリーンレンダリング用のリソースをリサイズ
	// オフスクリーンレンダリング用のリソースを作成
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = m_ResolutionWidth;
	resourceDesc.Height = m_ResolutionHeight;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Format = PixelFormat;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	// クリア値の設定
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = PixelFormat;
	clearValue.Color[0] = kClearColor[0];
	clearValue.Color[1] = kClearColor[1];
	clearValue.Color[2] = kClearColor[2];
	clearValue.Color[3] = kClearColor[3];
	for (int i = 0; i < static_cast<int>(GameRenderTextureType::GameRenderTextureTypeCount); i++)
	{
		GameRenderTextureType renderTexType = static_cast<GameRenderTextureType>(i);
		// リサイズ
		m_ResourceManager->RemakeColorBuffer(m_GameRenderTextures[renderTexType].m_BufferIndex, resourceDesc, &clearValue, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GameRenderTextures[renderTexType].m_Width = m_ResolutionWidth;
		m_GameRenderTextures[renderTexType].m_Height = m_ResolutionHeight;
	}
	// Scene用
	for(int i = 0; i < static_cast<int>(SceneRenderTextureType::SceneRenderTextureTypeCount); i++)
	{
		SceneRenderTextureType renderTexType = static_cast<SceneRenderTextureType>(i);
		// リサイズ
		m_ResourceManager->RemakeColorBuffer(m_SceneRenderTextures[renderTexType].m_BufferIndex, resourceDesc, &clearValue, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_SceneRenderTextures[renderTexType].m_Width = m_ResolutionWidth;
		m_SceneRenderTextures[renderTexType].m_Height = m_ResolutionHeight;
	}
}

void GraphicsEngine::BeginCommandContext(CommandContext* context)
{
	// コマンドリストのリセット
	context->Reset();
	// ディスクリプタヒープの設定
	context->SetDescriptorHeap(m_ResourceManager->GetSUVDHeap()->GetDescriptorHeap());
}

void GraphicsEngine::EndCommandContext(CommandContext* context, const QueueType& queue)
{
	// コマンドリストのクローズ
	context->Close();
	// コマンドリストの実行
	m_GraphicsCore->GetCommandManager()->ExecuteCommandList(context->GetCommandList(), queue);
	// シグナル
	m_GraphicsCore->GetCommandManager()->Signal(queue);
	// コマンドリストの返却
	m_GraphicsCore->GetCommandManager()->ReturnCommandContext(context);
}

void GraphicsEngine::WaitForGPU(const QueueType& queue)
{
	// GPUの完了待ち
	m_GraphicsCore->GetCommandManager()->WaitForFence(queue);
}

void GraphicsEngine::SetRenderTargets(CommandContext* context, DrawPass pass, RenderMode mode, bool isSetTarget)
{
	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ColorBuffer* renderTex = nullptr;
	DepthBuffer* depthTex = nullptr;
	switch (pass)
	{
	case GBuffers:
		if (mode == RenderMode::Game || mode == RenderMode::Release)
		{
			// GameGBufferRenderTextureの状態遷移
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameGBufferTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTV,DSVの設定
				depthTex = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex());
				/*context->SetRenderTarget(&rtvHandle, &dsvHandle);
				context->ClearRenderTarget(rtvHandle);
				context->ClearDepthStencil(dsvHandle);*/
				context->SetRenderTarget(renderTex, depthTex);
				context->ClearRenderTarget(renderTex);
				context->ClearDepthStencil(depthTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameGBufferTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		else if(mode == RenderMode::Scene)
		{
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneGBufferTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTV,DSVの設定
				depthTex = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex());
				//dsvHandle = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex())->GetDSVCpuHandle();
				//rtvHandle = renderTex->GetRTVCpuHandle();
				/*context->SetRenderTarget(&rtvHandle, &dsvHandle);
				context->ClearRenderTarget(rtvHandle);
				context->ClearDepthStencil(dsvHandle);*/
				context->SetRenderTarget(renderTex, depthTex);
				context->ClearRenderTarget(renderTex);
				context->ClearDepthStencil(depthTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				//renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneGBufferTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		else
		{
			theatria::Log::Write(LogLevel::Assert, "Unknown RenderMode for GBuffers pass");
		}
		break;
	case Lighting:
		if (mode == RenderMode::Game || mode == RenderMode::Release)
		{
			// GameLightingRenderTextureの状態遷移
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameLightingTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTVの設定
				/*rtvHandle = renderTex->GetRTVCpuHandle();
				context->SetRenderTarget(&rtvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameLightingTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		else if (mode == RenderMode::Scene)
		{
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneLightingTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTVの設定
				//rtvHandle = renderTex->GetRTVCpuHandle();
				/*context->SetRenderTarget(&rtvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneLightingTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		break;
	case Forward:
		if (mode == RenderMode::Game || mode == RenderMode::Release)
		{
			// GameForwardRenderTextureの状態遷移
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameForwardTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTV,DSVの設定
				depthTex = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex());
				/*dsvHandle = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex())->GetDSVCpuHandle();
				rtvHandle = renderTex->GetRTVCpuHandle();
				context->SetRenderTarget(&rtvHandle, &dsvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex, depthTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameForwardTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		else if (mode == RenderMode::Scene)
		{
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneForwardTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTV,DSVの設定
				depthTex = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex());
				/*dsvHandle = m_ResourceManager->GetBuffer<DepthBuffer>(m_DepthManager->GetDepthBufferIndex())->GetDSVCpuHandle();
				rtvHandle = renderTex->GetRTVCpuHandle();
				context->SetRenderTarget(&rtvHandle, &dsvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex, depthTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneForwardTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		break;
	case PostProcess:
		if (mode == RenderMode::Game || mode == RenderMode::Release)
		{
			// GamePostProcessRenderTextureの状態遷移
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GamePostProcessTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTVの設定
				/*rtvHandle = renderTex->GetRTVCpuHandle();
				context->SetRenderTarget(&rtvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GamePostProcessTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		else if (mode == RenderMode::Scene)
		{
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::ScenePostProcessTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTVの設定
				/*rtvHandle = renderTex->GetRTVCpuHandle();
				context->SetRenderTarget(&rtvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				/*renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::ScenePostProcessTexture].m_BufferIndex);
				context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		break;
	case SwapChainPass:
	{
		// SwapChainのBackBufferIndexを取得
		UINT backBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
		if (isSetTarget)
		{
			// StateをRenderTargetに変更	
			/*context->BarrierTransition(
				m_SwapChain->GetBuffer(backBufferIndex)->pResource.Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);*/
			// RTVの設定
			/*rtvHandle = m_SwapChain->GetBuffer(backBufferIndex)->m_RTVCpuHandle;
			context->SetRenderTarget(&rtvHandle);
			context->ClearRenderTarget(rtvHandle);*/
			context->SetRenderTarget(m_SwapChain->GetBuffer(backBufferIndex));
			context->ClearRenderTarget(m_SwapChain->GetBuffer(backBufferIndex));
		}
		else
		{
			// StateをPresentに変更
			//context->CheckResourceStateTransition(m_SwapChain->GetBuffer(backBufferIndex), D3D12_RESOURCE_STATE_PRESENT);
			/*context->BarrierTransition(
				m_SwapChain->GetBuffer(backBufferIndex)->pResource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);*/
		}
	}
			break;
	case EditorView:
		if (mode == RenderMode::Game)
		{
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameScreenTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTVの設定
				//rtvHandle = renderTex->GetRTVCpuHandle();
				/*context->SetRenderTarget(&rtvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameScreenTexture].m_BufferIndex);
				context->CheckResourceStateTransition(renderTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/

			}
		}
		else if (mode == RenderMode::Scene)
		{
			if (isSetTarget)
			{
				// StateをRenderTargetに変更	
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneScreenTexture].m_BufferIndex);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);*/
				// RTVの設定
				/*rtvHandle = renderTex->GetRTVCpuHandle();
				context->SetRenderTarget(&rtvHandle);
				context->ClearRenderTarget(rtvHandle);*/
				context->SetRenderTarget(renderTex);
				context->ClearRenderTarget(renderTex);
			}
			else
			{
				// StateをPixelShaderResourceに変更
				renderTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneScreenTexture].m_BufferIndex);
				context->CheckResourceStateTransition(renderTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				/*context->BarrierTransition(
					renderTex->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);*/
			}
		}
		else
		{
			Log::Write(LogLevel::Assert, "Unknown RenderMode for EditorView pass");
		}
		break;
	case PassCount:
		// ここに来るはずがない
		Log::Write(LogLevel::Assert, "Invalid DrawPass");
		break;
	default:
		// ここに来るはずがない
		Log::Write(LogLevel::Assert, "Invalid DrawPass");
		break;
	}
}

void GraphicsEngine::SetRenderState(CommandContext* context, ViewportType type)
{
	D3D12_VIEWPORT viewport;
	D3D12_RECT rect;
	switch (type)
	{
	case ViewportGame:
		// ビューポートの設定
		viewport = D3D12_VIEWPORT(
			0.0f, 0.0f,
			static_cast<float>(m_ResolutionWidth),
			static_cast<float>(m_ResolutionHeight),
			0.0f, 1.0f);
		// シザーレクトの設定
		rect = D3D12_RECT(
			0, 0,
			static_cast<LONG>(m_ResolutionWidth),
			static_cast<LONG>(m_ResolutionHeight));
		break;
	case ViewportSwapChain:
		// ビューポートの設定
		viewport = D3D12_VIEWPORT(
				0.0f, 0.0f,
				static_cast<float>(WinApp::GetWindowWidth()),
				static_cast<float>(WinApp::GetWindowHeight()),
				0.0f, 1.0f);
		// シザーレクトの設定
		rect = D3D12_RECT(
			0, 0,
			static_cast<LONG>(WinApp::GetWindowWidth()),
			static_cast<LONG>(WinApp::GetWindowHeight()));
		break;
	default:
		break;
	}
	context->SetViewport(viewport);
	context->SetScissorRect(rect);
	// プリミティブトポロジの設定
	context->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GraphicsEngine::DrawGBuffers([[maybe_unused]]ResourceManager& resourceManager, [[maybe_unused]] GameCore& gameCore, RenderMode mode)
{
	// コンテキスト取得
	CommandContext* context = GetCommandContext();
	// コマンドリスト開始
	BeginCommandContext(context);
	// レンダーターゲットの設定
	SetRenderTargets(context, DrawPass::GBuffers, mode, true);
	// 描画設定
	SetRenderState(context, ViewportGame);
	// レンダーターゲットのStateを戻す
	SetRenderTargets(context, DrawPass::GBuffers, mode, false);
	// コマンドリスト終了
	EndCommandContext(context, Graphics);
	// GPUの完了待ち
	WaitForGPU(Graphics);
}

void GraphicsEngine::DrawLighting([[maybe_unused]] ResourceManager& resourceManager, [[maybe_unused]] GameCore& gameCore, RenderMode mode)
{
	// コンテキスト取得
	CommandContext* context = GetCommandContext();
	// コマンドリスト開始
	BeginCommandContext(context);
	// レンダーターゲットの設定
	SetRenderTargets(context, DrawPass::Lighting, mode, true);
	// 描画設定
	SetRenderState(context, ViewportGame);
	// レンダーターゲットのStateを戻す
	SetRenderTargets(context, DrawPass::Lighting, mode, false);
	// コマンドリスト終了
	EndCommandContext(context, Graphics);
	// GPUの完了待ち
	WaitForGPU(Graphics);
}

void GraphicsEngine::DrawForward(ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode)
{
	// コンテキスト取得
	CommandContext* context = GetCommandContext();
	// コマンドリスト開始
	BeginCommandContext(context);
	// レンダーターゲットの設定
	SetRenderTargets(context, DrawPass::Forward, mode, true);
	// 描画設定
	SetRenderState(context, ViewportGame);
	// Skyboxの描画
	SkyboxRender(context, resourceManager, gameCore, mode);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetIntegratePSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetIntegratePSO().rootSignature.Get());
	// シーンに存在するメッシュを所持しているオブジェクトを全て描画
	uint32_t argsCount = 0;
	for (ModelData& modelData : resourceManager.GetModelManager()->GetModelDataContainer())
	{
		IConstantBuffer* cameraBuffer = nullptr;
		// メインカメラを取得
		if (mode == RenderMode::Game || mode == RenderMode::Release)
		{
			// カメラオブジェクトを取得
			GameObject* cameraObject = gameCore.GetGameWorld()->GetMainCamera();
			if (!cameraObject) { continue; }
			// カメラのバッファインデックスを取得
			CameraComponent* cameraComponent = gameCore.GetECSManager()->GetComponent<CameraComponent>(cameraObject->GetHandle().entity);
			if (!cameraComponent) { continue; }
			// カメラのバッファを取得
			cameraBuffer = resourceManager.GetBuffer<IConstantBuffer>(cameraComponent->bufferIndex);
			// カメラがないならスキップ
			if (!cameraBuffer) { continue; }
		}
		else// デバッグカメラ
		{
			// カメラのバッファを取得
			cameraBuffer = resourceManager.GetDebugCameraBuffer();
			// カメラがないならスキップ
			if (!cameraBuffer) { continue; }
		}
		// 登録されているTransformがないならスキップ
		if (modelData.useTransformList.empty()) { continue; }
		//// トランスフォーム統合バッファをセット
		//IStructuredBuffer* transformBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::Transform);
		//// UseTransformBufferをセット
		//IStructuredBuffer* useTransformBuffer = resourceManager.GetBuffer<IStructuredBuffer>(modelData.useTransformBufferIndex);
		//// ライトバッファをセット
		//IConstantBuffer* lightBuffer = resourceManager.GetLightBuffer();
		//// 環境情報バッファをセット
		//IConstantBuffer* envBuffer = resourceManager.GetEnvironmentBuffer();
		//// マテリアル統合バッファをセット
		//IStructuredBuffer* materialBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::Material);
		for (int i = 0; i < modelData.meshes.size(); i++)
		{
			// 頂点バッファビューをセット
			D3D12_VERTEX_BUFFER_VIEW* vbv = resourceManager.GetBuffer<IVertexBuffer>(modelData.meshes[i].vertexBufferIndex)->GetVertexBufferView();
			//context->SetVertexBuffers(0, 1, vbv);
			// インデックスバッファビューをセット
			D3D12_INDEX_BUFFER_VIEW* ibv = resourceManager.GetBuffer<IIndexBuffer>(modelData.meshes[i].indexBufferIndex)->GetIndexBufferView();
			//context->SetIndexBuffer(ibv);
			// カメラバッファをセット
			// context->SetGraphicsRootConstantBufferView(0, cameraBuffer->GetResource()->GetGPUVirtualAddress());
			// トランスフォーム統合バッファをセット
			IStructuredBuffer* transformBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::Transform);
			//context->SetGraphicsRootShaderResourceView(1, transformBuffer->GetResource()->GetGPUVirtualAddress());
			// UseTransformBufferをセット
			IStructuredBuffer* useTransformBuffer = resourceManager.GetBuffer<IStructuredBuffer>(modelData.useTransformBufferIndex);
			//context->SetGraphicsRootShaderResourceView(2, useTransformBuffer->GetResource()->GetGPUVirtualAddress());
			// モデルのボーン行列Bufferをセット
			IStructuredBuffer* boneBuffer = resourceManager.GetBuffer<IStructuredBuffer>(modelData.boneMatrixBufferIndex);
			//context->SetGraphicsRootShaderResourceView(3, boneBuffer->GetResource()->GetGPUVirtualAddress());
			// SkinningInfluenceBufferをセット
			IStructuredBuffer* skinningInfluenceBuffer = resourceManager.GetBuffer<IStructuredBuffer>(modelData.meshes[i].influenceBufferIndex);
			//context->SetGraphicsRootShaderResourceView(4, skinningInfluenceBuffer->GetResource()->GetGPUVirtualAddress());
			// SkinningInfoBufferをセット
			IConstantBuffer* skinningInfoBuffer = resourceManager.GetBuffer<IConstantBuffer>(modelData.meshes[i].skinInfoBufferIndex);
			//context->SetGraphicsRootConstantBufferView(5, skinningInfoBuffer->GetResource()->GetGPUVirtualAddress());
			// ライトバッファをセット
			IConstantBuffer* lightBuffer = resourceManager.GetLightBuffer();
			//context->SetGraphicsRootConstantBufferView(6, lightBuffer->GetResource()->GetGPUVirtualAddress());
			// 環境情報バッファをセット
			IConstantBuffer* envBuffer = resourceManager.GetEnvironmentBuffer();
			//context->SetGraphicsRootConstantBufferView(7, envBuffer->GetResource()->GetGPUVirtualAddress());
			// PS用トランスフォーム統合バッファをセット
			//context->SetGraphicsRootShaderResourceView(8, transformBuffer->GetResource()->GetGPUVirtualAddress());
			// マテリアル統合バッファをセット
			IStructuredBuffer* materialBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::Material);
			//context->SetGraphicsRootShaderResourceView(9, materialBuffer->GetResource()->GetGPUVirtualAddress());
			// 引数をセット
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root0 = cameraBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root1 = transformBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root2 = useTransformBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root3 = boneBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root4 = skinningInfluenceBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root5 = skinningInfoBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root6 = lightBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root7 = envBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root8 = transformBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].root9 = materialBuffer->GetResource()->GetGPUVirtualAddress();
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].vbv = *vbv;
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].ibv = *ibv;
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].draw.InstanceCount = static_cast<UINT>(modelData.useTransformList.size());
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].draw.StartIndexLocation = 0;
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].draw.BaseVertexLocation = 0;
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].draw.StartInstanceLocation = 0;
			m_PipelineManager->GetIntegratePSO().argsBuffer.mappedData[argsCount].draw.IndexCountPerInstance = static_cast<UINT>(modelData.meshes[i].indices.size());
			/*modelData.argsBuffer.mappedData[0].root0 = cameraBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root1 = transformBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root2 = useTransformBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root3 = boneBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root4 = skinningInfluenceBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root5 = skinningInfoBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root6 = lightBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root7 = envBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root8 = transformBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].root9 = materialBuffer->GetResource()->GetGPUVirtualAddress();
			modelData.argsBuffer.mappedData[0].vbv = *vbv;
			modelData.argsBuffer.mappedData[0].ibv = *ibv;
			modelData.argsBuffer.mappedData[0].draw.InstanceCount = static_cast<UINT>(modelData.useTransformList.size());
			modelData.argsBuffer.mappedData[0].draw.StartIndexLocation = 0;
			modelData.argsBuffer.mappedData[0].draw.BaseVertexLocation = 0;
			modelData.argsBuffer.mappedData[0].draw.StartInstanceLocation = 0;
			modelData.argsBuffer.mappedData[0].draw.IndexCountPerInstance = static_cast<UINT>(modelData.meshes[i].indices.size());*/
			argsCount++;
			//// 引数バッファを更新
			//context->BarrierTransition(
			//	modelData.argsBuffer.h_Default->GetResource(),
			//	D3D12_RESOURCE_STATE_COMMON,
			//	D3D12_RESOURCE_STATE_COPY_DEST);
			//// 引数バッファにアップロード
			////const UINT64 sizeToCopy = m_PipelineManager->GetIntegratePSO().argsBuffer.byteStride * argsCount;
			//context->CopyBufferRegion(
			//	modelData.argsBuffer.h_Default->GetResource(), 0,
			//	modelData.argsBuffer.h_Upload->GetResource(), 0,
			//	static_cast<UINT>(modelData.argsBuffer.byteStride));
			//// 引数バッファState遷移
			//context->BarrierTransition(
			//	modelData.argsBuffer.h_Default->GetResource(),
			//	D3D12_RESOURCE_STATE_COPY_DEST,
			//	D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			//// 配列テクスチャのためヒープをセット
			//context->SetGraphicsRootDescriptorTable(10, resourceManager.GetSUVDHeap()->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			//// 配列CubeTexture
			//PixelBuffer* skyboxTexture = resourceManager.GetBuffer<PixelBuffer>(resourceManager.GetTextureManager()->GetTextureID(resourceManager.GetSkyboxTextureName()));
			//context->SetGraphicsRootDescriptorTable(11, skyboxTexture->GetSRVGpuHandle());
			//// 各ResourceState遷移
			////context->BarrierTransition(cameraBuffer->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			////context->BarrierTransition(transformBuffer->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			//// インスタンス数を取得
			////UINT numInstance = static_cast<UINT>(modelData.useTransformList.size());
			//// DrawCall
			////context->DrawIndexedInstanced(static_cast<UINT>(modelData.meshes[i].indices.size()), numInstance, 0, 0, 0);
			//// IndirectDrawCall
			//context->ExecuteIndirect(
			//	m_PipelineManager->GetIntegratePSO().commandSignature.Get(),
			//	1,
			//	modelData.argsBuffer.h_Default->GetResource(),
			//	0,
			//	nullptr,
			//	0);
			//// 引数バッファStateを戻す
			//context->BarrierTransition(
			//	modelData.argsBuffer.h_Default->GetResource(),
			//	D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
			//	D3D12_RESOURCE_STATE_COMMON);
		}
	}
	if (argsCount != 0)
	{
		// 引数バッファを更新
		/*context->BarrierTransition(
			m_PipelineManager->GetIntegratePSO().argsBuffer.h_Default->GetResource(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST);*/
		// 引数バッファにアップロード
		const UINT64 sizeToCopy = m_PipelineManager->GetIntegratePSO().argsBuffer.byteStride * argsCount;
		context->CopyBufferRegion(
			m_PipelineManager->GetIntegratePSO().argsBuffer.h_Default.get(), 0,
			m_PipelineManager->GetIntegratePSO().argsBuffer.h_Upload.get(), 0,
			static_cast<UINT>(sizeToCopy));
		// 引数バッファState遷移
		/*context->BarrierTransition(
			m_PipelineManager->GetIntegratePSO().argsBuffer.h_Default->GetResource(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);*/
		// 配列テクスチャのためヒープをセット
		context->SetGraphicsRootDescriptorTable(10, resourceManager.GetSUVDHeap()->GetDescriptorHeap());
		// 配列CubeTexture
		PixelBuffer* skyboxTexture = resourceManager.GetBuffer<PixelBuffer>(resourceManager.GetTextureManager()->GetTextureID(resourceManager.GetSkyboxTextureName()));
		context->SetGraphicsRootDescriptorTable(11, skyboxTexture, ViewType::ShaderResourceView);
		// 各ResourceState遷移
		//context->BarrierTransition(cameraBuffer->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		//context->BarrierTransition(transformBuffer->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		// インスタンス数を取得
		//UINT numInstance = static_cast<UINT>(modelData.useTransformList.size());
		// DrawCall
		//context->DrawIndexedInstanced(static_cast<UINT>(modelData.meshes[i].indices.size()), numInstance, 0, 0, 0);
		const UINT64 byteStride = m_PipelineManager->GetIntegratePSO().argsBuffer.byteStride;
		const UINT64 bufferSize = m_PipelineManager->GetIntegratePSO().argsBuffer.totalBytes;
		const UINT64 argOffset = 0; // 使ってる開始位置
		const UINT64 capacity = (bufferSize - argOffset) / byteStride;
		const UINT    maxExec = (UINT)std::min<UINT64>(argsCount, capacity);
		// IndirectDrawCall
		context->ExecuteIndirect(
			m_PipelineManager->GetIntegratePSO().commandSignature.Get(),
			maxExec,
			m_PipelineManager->GetIntegratePSO().argsBuffer.h_Default.get(),
			0,
			nullptr,
			0);
		// 引数バッファStateを戻す
		/*context->BarrierTransition(
			m_PipelineManager->GetIntegratePSO().argsBuffer.h_Default->GetResource(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
			D3D12_RESOURCE_STATE_COMMON);*/
	}
	//}
//}
// ラインの描画
	for (uint32_t i = 0; i < 1; i++)
	{
		// プリミティブトポロジの設定
		context->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		// パイプラインセット
		context->SetGraphicsPipelineState(m_PipelineManager->GetLinePSO().pso.Get());
		// ルートシグネチャセット
		context->SetGraphicsRootSignature(m_PipelineManager->GetLinePSO().rootSignature.Get());
		IConstantBuffer* cameraBuffer = nullptr;
		// メインカメラを取得
		if (mode == RenderMode::Game || mode == RenderMode::Release)
		{
			// カメラオブジェクトを取得
			GameObject* cameraObject = gameCore.GetGameWorld()->GetMainCamera();
			if (!cameraObject) { continue; }
			// カメラのバッファインデックスを取得
			CameraComponent* cameraComponent = gameCore.GetECSManager()->GetComponent<CameraComponent>(cameraObject->GetHandle().entity);
			if (!cameraComponent) { continue; }
			// カメラのバッファを取得
			cameraBuffer = resourceManager.GetBuffer<IConstantBuffer>(cameraComponent->bufferIndex);
			// カメラがないならスキップ
			if (!cameraBuffer) { continue; }
		}
		else// デバッグカメラ
		{
			// カメラのバッファを取得
			cameraBuffer = resourceManager.GetDebugCameraBuffer();
			// カメラがないならスキップ
			if (!cameraBuffer) { continue; }
		}
		// MapIDが0（使われていない）ならスキップ
		if (!resourceManager.GetIntegrationData(IntegrationDataType::Line)->GetActiveCount()) { continue; }
		// 頂点バッファビューをセット
		D3D12_VERTEX_BUFFER_VIEW* vbv = resourceManager.GetLineIntegrationBuffer()->GetVertexBufferView();
		context->SetVertexBuffers(0, 1, vbv);
		// カメラバッファをセット
		context->SetGraphicsRootConstantBufferView(0, cameraBuffer);
		// DrawCall
		context->DrawInstanced(static_cast<UINT>(resourceManager.GetIntegrationData(IntegrationDataType::Line)->GetActiveCount()) * 2, 1, 0, 0);
	}
	// パーティクル
	DrawParticles(context, resourceManager, gameCore, mode);
	// EffectEditor
	EffectEditorDraw(context, resourceManager, gameCore, mode);
	// UI
	DrawUI(context, resourceManager, gameCore, mode);
	// レンダーターゲットのStateを戻す
	SetRenderTargets(context, DrawPass::Forward, mode, false);
	// コマンドリスト終了
	EndCommandContext(context, Graphics);
	// GPUの完了待ち
	WaitForGPU(Graphics);
}

void GraphicsEngine::DrawPostProcess([[maybe_unused]] ResourceManager& resourceManager, [[maybe_unused]]GameCore& gameCore, RenderMode mode)
{
	// コンテキスト取得
	CommandContext* context = GetCommandContext();
	// コマンドリスト開始
	BeginCommandContext(context);
	// レンダーターゲットの設定
	SetRenderTargets(context, DrawPass::PostProcess, mode, true);
	// 描画設定
	SetRenderState(context, ViewportGame);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetScreenCopyPSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetScreenCopyPSO().rootSignature.Get());
	// フォワードの結果をセット
	ColorBuffer* forwardTex = nullptr;
	if(mode == RenderMode::Game || mode == RenderMode::Release)
	{
		forwardTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GameForwardTexture].m_BufferIndex);
	}
	else if (mode == RenderMode::Scene)
	{
		forwardTex = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::SceneForwardTexture].m_BufferIndex);
	}
	else
	{
		theatria::Log::Write(LogLevel::Assert, "Unknown RenderMode for PostProcess pass");
		return;
	}
	// オフスクリーンレンダリングテクスチャをセット
	context->SetGraphicsRootDescriptorTable(0, forwardTex, ViewType::ShaderResourceView);
	// DrawCall
	context->DrawInstanced(3, 1, 0, 0);
	// レンダーターゲットのStateを戻す
	SetRenderTargets(context, DrawPass::PostProcess, mode, false);
	// コマンドリスト終了
	EndCommandContext(context, Graphics);
	// GPUの完了待ち
	WaitForGPU(Graphics);
}

void GraphicsEngine::DrawEditorView([[maybe_unused]] ResourceManager& resourceManager, [[maybe_unused]] GameCore& gameCore, RenderMode mode)
{
	if (mode != RenderMode::Game && mode != RenderMode::Scene) { return; }
	// コンテキスト取得
	CommandContext* context = GetCommandContext();
	// コマンドリスト開始
	BeginCommandContext(context);
	// レンダーターゲットの設定
	SetRenderTargets(context, DrawPass::EditorView, mode, true);
	// 描画設定
	SetRenderState(context, ViewportGame);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetScreenCopyPSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetScreenCopyPSO().rootSignature.Get());
	// ポストプロセスの結果をセット
	ColorBuffer* postProcess = nullptr;
	if (mode == RenderMode::Game || mode == RenderMode::Release)
	{
		postProcess = m_ResourceManager->GetBuffer<ColorBuffer>(m_GameRenderTextures[GameRenderTextureType::GamePostProcessTexture].m_BufferIndex);
	}
	else if (mode == RenderMode::Scene)
	{
		postProcess = m_ResourceManager->GetBuffer<ColorBuffer>(m_SceneRenderTextures[SceneRenderTextureType::ScenePostProcessTexture].m_BufferIndex);
	}
	else
	{
		theatria::Log::Write(LogLevel::Assert, "Unknown RenderMode for PostProcess pass");
		return;
	}
	// オフスクリーンレンダリングテクスチャをセット
	context->SetGraphicsRootDescriptorTable(0, postProcess, ViewType::ShaderResourceView);
	// DrawCall
	context->DrawInstanced(3, 1, 0, 0);
	// レンダーターゲットのStateを戻す
	SetRenderTargets(context, DrawPass::EditorView, mode, false);
	// コマンドリスト終了
	EndCommandContext(context, Graphics);
	// GPUの完了待ち
	WaitForGPU(Graphics);
}

void GraphicsEngine::CreateDepthBuffer()
{
	// DepthBufferの生成
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = m_ResolutionWidth;
	resourceDesc.Height = m_ResolutionHeight;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	m_DepthManager->SetDepthBufferIndex(m_ResourceManager->CreateDepthBuffer(resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void GraphicsEngine::CreateOffscreenBuffer()
{
	// オフスクリーンレンダリング用のリソースを作成
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = m_ResolutionWidth;
	resourceDesc.Height = m_ResolutionHeight;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Format = PixelFormat;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	// クリア値の設定
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = PixelFormat;
	clearValue.Color[0] = kClearColor[0];
	clearValue.Color[1] = kClearColor[1];
	clearValue.Color[2] = kClearColor[2];
	clearValue.Color[3] = kClearColor[3];
	// Game用
	for (int i = 0; i < static_cast<int>(GameRenderTextureType::GameRenderTextureTypeCount); i++)
	{
		GameRenderTextureType renderTexType = static_cast<GameRenderTextureType>(i);
		m_GameRenderTextures[renderTexType].m_BufferIndex = m_ResourceManager->CreateColorBuffer(resourceDesc, &clearValue, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GameRenderTextures[renderTexType].m_Width = resourceDesc.Width;
		m_GameRenderTextures[renderTexType].m_Height = resourceDesc.Height;
	}
	// Scene用
	for (int i = 0; i < static_cast<int>(SceneRenderTextureType::SceneRenderTextureTypeCount); i++)
	{
		SceneRenderTextureType renderTexType = static_cast<SceneRenderTextureType>(i);
		m_SceneRenderTextures[renderTexType].m_BufferIndex = m_ResourceManager->CreateColorBuffer(resourceDesc, &clearValue, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_SceneRenderTextures[renderTexType].m_Width = resourceDesc.Width;
		m_SceneRenderTextures[renderTexType].m_Height = resourceDesc.Height;
	}
}

void GraphicsEngine::CreateDebugDepthBuffer()
{
	// DepthBufferの生成
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = m_ResolutionWidth;
	resourceDesc.Height = m_ResolutionHeight;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	// デバッグ用DepthBufferの生成
	m_DepthManager->SetDebugDepthBufferIndex(m_ResourceManager->CreateDepthBuffer(resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void GraphicsEngine::DrawParticles(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode)
{
	// プリミティブトポロジの設定
	context->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetParticlePSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetParticlePSO().rootSignature.Get());

	for (const auto& scene: gameCore.GetGameWorld()->GetWorldContainer())
	{
		for (const auto& objects : scene)
		{
			for (const auto& object : objects)
			{
				if (object->GetType() != ObjectType::ParticleSystem) { continue; }
				// EmitterComponentを取得
				EmitterComponent* emitterComponent = gameCore.GetECSManager()->GetComponent<EmitterComponent>(object->GetHandle().entity);
				if (!emitterComponent) { continue; }
				//if (!emitterComponent->particleID) { continue; }
				// ParticleComponentを取得
				ParticleComponent* particleComponent = gameCore.GetECSManager()->GetComponent<ParticleComponent>(object->GetHandle().entity);
				if (!particleComponent) { continue; }
				MeshFilterComponent* meshFilterComponent = gameCore.GetECSManager()->GetComponent<MeshFilterComponent>(object->GetHandle().entity);
				if (!meshFilterComponent) { continue; }
				MeshRendererComponent* meshRendererComponent = gameCore.GetECSManager()->GetComponent<MeshRendererComponent>(object->GetHandle().entity);
				if (!meshRendererComponent) { continue; }
				IConstantBuffer* cameraBuffer = nullptr;
				// メインカメラを取得
				if (mode == RenderMode::Game || mode == RenderMode::Release)
				{
					// カメラオブジェクトを取得
					GameObject* cameraObject = gameCore.GetGameWorld()->GetMainCamera();
					if (!cameraObject) { continue; }
					// カメラのバッファインデックスを取得
					CameraComponent* cameraComponent = gameCore.GetECSManager()->GetComponent<CameraComponent>(cameraObject->GetHandle().entity);
					if (!cameraComponent) { continue; }
					// カメラのバッファを取得
					cameraBuffer = resourceManager.GetBuffer<IConstantBuffer>(cameraComponent->bufferIndex);
					// カメラがないならスキップ
					if (!cameraBuffer) { continue; }
				}
				else// デバッグカメラ
				{
					// カメラのバッファを取得
					cameraBuffer = resourceManager.GetDebugCameraBuffer();
					// カメラがないならスキップ
					if (!cameraBuffer) { continue; }
				}
				// 
				ModelData* modelData = resourceManager.GetModelManager()->GetModelData(meshFilterComponent->modelName);
				if (!modelData) { break; }
				// VBVをセット
				D3D12_VERTEX_BUFFER_VIEW* vbv = resourceManager.GetBuffer<IVertexBuffer>(modelData->meshes[0].vertexBufferIndex)->GetVertexBufferView();
				context->SetVertexBuffers(0, 1, vbv);
				// IBVをセット
				D3D12_INDEX_BUFFER_VIEW* ibv = resourceManager.GetBuffer<IIndexBuffer>(modelData->meshes[0].indexBufferIndex)->GetIndexBufferView();
				context->SetIndexBuffer(ibv);
				// カメラバッファをセット
				context->SetGraphicsRootConstantBufferView(0, cameraBuffer);
				// パーティクルバッファをセット
				IRWStructuredBuffer* particleBuffer = resourceManager.GetBuffer<IRWStructuredBuffer>(particleComponent->bufferIndex);
				context->SetGraphicsRootDescriptorTable(1, particleBuffer, ViewType::UnorderedAccessView);
				// マテリアル統合バッファをセット
				IStructuredBuffer* materialBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::Material);
				context->SetGraphicsRootDescriptorTable(2, materialBuffer, ViewType::ShaderResourceView);
				// 配列テクスチャのためヒープをセット
				context->SetGraphicsRootDescriptorTable(3, resourceManager.GetSUVDHeap()->GetDescriptorHeap());
				// DrawCall
				context->DrawIndexedInstanced(static_cast<UINT>(modelData->meshes[0].indices.size()), particleComponent->count, 0, 0, 0);
			}
		}
	}
}

void GraphicsEngine::EffectEditorDraw(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode)
{
	resourceManager;
	gameCore;
	mode;
	// GPU待機
	WaitForGPU(Compute);
	// レンダーターゲットの設定
	//SetRenderTargets(context, DrawPass::GBuffers, RenderMode::Editor);
	// 描画設定
	//SetRenderState(context, ViewportGame);
	// プリミティブトポロジの設定
	context->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetEffectSpritePSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetEffectSpritePSO().rootSignature.Get());
	IConstantBuffer* cameraBuffer = nullptr;
	// メインカメラを取得
	if (mode == RenderMode::Game || mode == RenderMode::Release)
	{
		// カメラオブジェクトを取得
		GameObject* cameraObject = gameCore.GetGameWorld()->GetMainCamera();
		if (!cameraObject) return;
		// カメラのバッファインデックスを取得
		CameraComponent* cameraComponent = gameCore.GetECSManager()->GetComponent<CameraComponent>(cameraObject->GetHandle().entity);
		if (!cameraComponent) { return; }
		// カメラのバッファを取得
		cameraBuffer = resourceManager.GetBuffer<IConstantBuffer>(cameraComponent->bufferIndex);
		// カメラがないならスキップ
		if (!cameraBuffer) { return; }
	} else// デバッグカメラ
	{
		// カメラのバッファを取得
		cameraBuffer = resourceManager.GetDebugCameraBuffer();
		// カメラがないならスキップ
		if (!cameraBuffer) { return; }
	}
	// 板ポリ取得
	ModelData* modelData = resourceManager.GetModelManager()->GetModelData(L"Plane");
	if (!modelData) { return; }
	// VBVをセット
	D3D12_VERTEX_BUFFER_VIEW* vbv = resourceManager.GetBuffer<IVertexBuffer>(modelData->meshes[0].vertexBufferIndex)->GetVertexBufferView();
	context->SetVertexBuffers(0, 1, vbv);
	// IBVをセット
	D3D12_INDEX_BUFFER_VIEW* ibv = resourceManager.GetBuffer<IIndexBuffer>(modelData->meshes[0].indexBufferIndex)->GetIndexBufferView();
	context->SetIndexBuffer(ibv);
	// カメラバッファをセット
	context->SetGraphicsRootConstantBufferView(0, cameraBuffer);
	// RootBufferをセット
	IStructuredBuffer* rootBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::EffectRootInt);
	context->SetGraphicsRootDescriptorTable(1, rootBuffer, ViewType::ShaderResourceView);
	// NodeBufferをセット
	IStructuredBuffer* nodeBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::EffectNodeInt);
	context->SetGraphicsRootDescriptorTable(2, nodeBuffer, ViewType::ShaderResourceView);
	// SpriteBufferをセット
	IStructuredBuffer* spriteBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::EffectSpriteInt);
	context->SetGraphicsRootDescriptorTable(3, spriteBuffer, ViewType::ShaderResourceView);
	// EffectParticleBufferをセット
	IRWStructuredBuffer* effectParticleBuffer = resourceManager.GetEffectParticleBuffer();
	context->SetGraphicsRootDescriptorTable(4, effectParticleBuffer, ViewType::UnorderedAccessView);
	// NodeBufferをセット
	context->SetGraphicsRootDescriptorTable(5, nodeBuffer, ViewType::ShaderResourceView);
	// EffectParticleBufferをセット
	context->SetGraphicsRootDescriptorTable(6, effectParticleBuffer, ViewType::UnorderedAccessView);
	// TextureBufferをセット
	context->SetGraphicsRootDescriptorTable(7, resourceManager.GetSUVDHeap()->GetDescriptorHeap());
	// DrawCall
	context->DrawIndexedInstanced(static_cast<UINT>(modelData->meshes[0].indices.size()), 128 * 1024, 0, 0, 0);
}

void GraphicsEngine::DrawUI(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode)
{
	if (resourceManager.GetUIContainer()->GetUseList().empty())
	{
		return;
	}

	// プリミティブトポロジの設定
	context->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetUIPSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetUIPSO().rootSignature.Get());

	// UIComponentを取得
	IConstantBuffer* cameraBuffer = nullptr;
	// メインカメラを取得
	if (mode == RenderMode::Game || mode == RenderMode::Release)
	{
		// カメラオブジェクトを取得
		GameObject* cameraObject = gameCore.GetGameWorld()->GetMainCamera();
		if (!cameraObject) { return; }
		// カメラのバッファインデックスを取得
		CameraComponent* cameraComponent = gameCore.GetECSManager()->GetComponent<CameraComponent>(cameraObject->GetHandle().entity);
		if (!cameraComponent) { return; }
		// カメラのバッファを取得
		cameraBuffer = resourceManager.GetBuffer<IConstantBuffer>(cameraComponent->bufferIndex);
		// カメラがないならスキップ
		if (!cameraBuffer) { return; }
	} else// デバッグカメラ
	{
		// カメラのバッファを取得
		cameraBuffer = resourceManager.GetDebugCameraBuffer();
		// カメラがないならスキップ
		if (!cameraBuffer) { return; }
	}
	// UIの頂点バッファを取得
	UIData& uiData = resourceManager.GetUIContainer()->GetUIData(0);
	// VBVをセット
	D3D12_VERTEX_BUFFER_VIEW* vbv = resourceManager.GetBuffer<IVertexBuffer>(uiData.vertexBufferIndex)->GetVertexBufferView();
	context->SetVertexBuffers(0, 1, vbv);
	// IBVをセット
	D3D12_INDEX_BUFFER_VIEW* ibv = resourceManager.GetBuffer<IIndexBuffer>(uiData.indexBufferIndex)->GetIndexBufferView();
	context->SetIndexBuffer(ibv);
	// UI統合バッファをセット
	IStructuredBuffer* uiBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::UISprite);
	context->SetGraphicsRootDescriptorTable(0, uiBuffer, ViewType::ShaderResourceView);
	// UIUseListをセット
	IStructuredBuffer* useUIBuffer = resourceManager.GetBuffer<IStructuredBuffer>(resourceManager.GetUIContainer()->GetUseListBufferIndex());
	context->SetGraphicsRootDescriptorTable(1, useUIBuffer, ViewType::ShaderResourceView);
	// マテリアル統合バッファをセット
	IStructuredBuffer* materialBuffer = resourceManager.GetIntegrationBuffer(IntegrationDataType::Material);
	context->SetGraphicsRootDescriptorTable(2, materialBuffer, ViewType::ShaderResourceView);
	// 配列テクスチャのためヒープをセット
	context->SetGraphicsRootDescriptorTable(3, resourceManager.GetSUVDHeap()->GetDescriptorHeap());
	// DrawCall
	context->DrawIndexedInstanced(6, static_cast<UINT>(resourceManager.GetUIContainer()->GetUseList().size()), 0, 0, 0);

}

void GraphicsEngine::SkyboxRender(CommandContext* context, ResourceManager& resourceManager, GameCore& gameCore, RenderMode mode)
{
	if (resourceManager.GetSkyboxTextureName().empty())
	{
		return;
	}

	// プリミティブトポロジの設定
	context->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// パイプラインセット
	context->SetGraphicsPipelineState(m_PipelineManager->GetSkyboxPSO().pso.Get());
	// ルートシグネチャセット
	context->SetGraphicsRootSignature(m_PipelineManager->GetSkyboxPSO().rootSignature.Get());
	IConstantBuffer* cameraBuffer = nullptr;
	// メインカメラを取得
	if (mode == RenderMode::Game || mode == RenderMode::Release)
	{
		// カメラオブジェクトを取得
		GameObject* cameraObject = gameCore.GetGameWorld()->GetMainCamera();
		if (!cameraObject) { return; }
		// カメラのバッファインデックスを取得
		CameraComponent* cameraComponent = gameCore.GetECSManager()->GetComponent<CameraComponent>(cameraObject->GetHandle().entity);
		if (!cameraComponent) { return; }
		// カメラのバッファを取得
		cameraBuffer = resourceManager.GetBuffer<IConstantBuffer>(cameraComponent->bufferIndex);
		// カメラがないならスキップ
		if (!cameraBuffer) { return; }
	}
	else// デバッグカメラ
	{
		// カメラのバッファを取得
		cameraBuffer = resourceManager.GetDebugCameraBuffer();
		// カメラがないならスキップ
		if (!cameraBuffer) { return; }
	}
	// Skyboxの頂点バッファを取得
	ModelData* model = resourceManager.GetModelManager()->GetModelData(L"Skybox");
	// VBVをセット
	D3D12_VERTEX_BUFFER_VIEW* vbv = resourceManager.GetBuffer<IVertexBuffer>(model->meshes[0].vertexBufferIndex)->GetVertexBufferView();
	context->SetVertexBuffers(0, 1, vbv);
	// IBVをセット
	D3D12_INDEX_BUFFER_VIEW* ibv = resourceManager.GetBuffer<IIndexBuffer>(model->meshes[0].indexBufferIndex)->GetIndexBufferView();
	context->SetIndexBuffer(ibv);
	// カメラバッファをセット
	context->SetGraphicsRootConstantBufferView(0, cameraBuffer);
	// Skyboxのテクスチャをセット
	TextureData* skyboxTexture = resourceManager.GetTextureManager()->GetTextureData(resourceManager.GetSkyboxTextureName());
	PixelBuffer* skyboxPixelBuffer = resourceManager.GetBuffer<PixelBuffer>(skyboxTexture->bufferIndex);
	context->SetGraphicsRootDescriptorTable(1, skyboxPixelBuffer, ViewType::ShaderResourceView);
	// DrawCall
	context->DrawIndexedInstanced(static_cast<UINT>(model->meshes[0].indices.size()), 1, 0, 0, 0);

}
