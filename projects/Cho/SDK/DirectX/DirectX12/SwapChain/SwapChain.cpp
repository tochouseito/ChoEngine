#include "pch.h"
#include "SwapChain.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;

SwapChain::SwapChain(IDXGIFactory7* dxgiFactory, GraphicsEngine* graphicsEngine, ID3D12CommandQueue* queue, CommandManager* commandManagerPtr, const HWND& hwnd, const UINT64& width, const UINT& height)
	:m_GraphicsEngine(graphicsEngine), m_CommandManager(commandManagerPtr)
{
	CreateSwapChain(dxgiFactory, queue,  hwnd, width, height);
}

void SwapChain::CreateSwapChain(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* queue, const HWND& hwnd, const UINT64& width, const UINT& height)
{
	HRESULT hr;

	// スワップチェーンを生成する
	m_Desc.Width = static_cast<UINT>(width);			  // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	m_Desc.Height = height;								  // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	m_Desc.Format = PixelFormat;						  // 色の形式
	m_Desc.SampleDesc.Count = 1;                          // マルチサンプルしない
	m_Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
	m_Desc.BufferCount = bufferCount;                     // ダブルバッファ
	m_Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    // モニタにうつしたら、中身を破棄
	m_Desc.Flags =
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING |
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;// ティアリングサポート

	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
		queue,
		hwnd,
		&m_Desc,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(m_SwapChain.GetAddressOf())
	);

	Log::Write(LogLevel::Assert, "SwapChain created.", hr);

	// リフレッシュレートを取得。floatで取るのは大変なので大体あってれば良いので整数で。
	HDC hdc = GetDC(hwnd);
	m_RefreshRate = GetDeviceCaps(hdc, VREFRESH);
	ReleaseDC(hwnd, hdc);

	// VSync共存型FPS固定のためにレイテンシ1
	m_SwapChain->SetMaximumFrameLatency(1);

	// OSが行うAlt+Enterのフルスクリーンは制御不能なので禁止
	dxgiFactory->MakeWindowAssociation(
		hwnd,
		DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER
	);
}

void SwapChain::CreateResource(ID3D12Device8* device, ResourceManager* resourceManager)
{
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = m_Desc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;// 2dテクスチャとして書き込む
	for (uint32_t i = 0; i < bufferCount; ++i)
	{
		m_BufferData[i] = std::make_unique<SwapChainBuffer>();
		m_BufferData[i]->pResource.Attach(nullptr);
		ID3D12Resource* pResource = nullptr;
		HRESULT hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&pResource));
		Log::Write(LogLevel::Assert, "SwapChain buffer created.", hr);
		m_BufferData[i]->pResource.Attach(pResource);
		m_BufferData[i]->backBufferIndex = i;
		m_BufferData[i]->m_RTVHandleIndex = resourceManager->GetRTVDHeap()->Allocate();
		m_BufferData[i]->m_RTVCpuHandle = resourceManager->GetRTVDHeap()->GetCPUDescriptorHandle(m_BufferData[i]->m_RTVHandleIndex.value());
		// Viewの生成
		device->CreateRenderTargetView(
			m_BufferData[i]->pResource.Get(),
			&rtvDesc,
			m_BufferData[i]->m_RTVCpuHandle
		);
	}
}

void SwapChain::Present()
{
	// リソースステートのチェック
	UINT backBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
	SwapChainBuffer* buffer = m_BufferData[backBufferIndex].get();
	if(buffer->m_ResourceState != D3D12_RESOURCE_STATE_PRESENT)
	{
		// コマンドリスト取得
		CommandContext* context = m_CommandManager->GetCommandContext();
		// コマンドリスト開始
		m_GraphicsEngine->BeginCommandContext(context);
		// リソースバリア設定
		context->CheckResourceStateTransition(buffer, D3D12_RESOURCE_STATE_PRESENT);
		// コマンドリスト終了
		m_GraphicsEngine->EndCommandContext(context, Graphics);
		// GPUの完了待ち
		m_GraphicsEngine->WaitForGPU(Graphics);
	}
	m_SwapChain->Present(1, 0);
}

void SwapChain::Resize(ID3D12Device8* device, const UINT64& width, const UINT& height)
{
	for (uint32_t i = 0; i < bufferCount; ++i)
	{
		m_BufferData[i]->pResource.Reset();
		m_BufferData[i]->pResource = nullptr;
	}
	m_Desc.Width = static_cast<UINT>(width);  // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	m_Desc.Height = height;                   // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	HRESULT hr = m_SwapChain->ResizeBuffers(bufferCount, static_cast<UINT>(width), height, PixelFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
	Log::Write(LogLevel::Assert, "SwapChain resized.", hr);
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = m_Desc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;// 2dテクスチャとして書き込む
	for (uint32_t i = 0; i < bufferCount; ++i)
	{
		hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BufferData[i]->pResource));
		Log::Write(LogLevel::Assert, "SwapChain buffer created.", hr);
		// Viewの生成
		device->CreateRenderTargetView(
			m_BufferData[i]->pResource.Get(),
			&rtvDesc,
			m_BufferData[i]->m_RTVCpuHandle
		);
	}
}

