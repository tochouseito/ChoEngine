#include "pch.h"
#include "ChoEngine.h"

// Windowアプリケーション
#include "Cho/OS/Windows/WinApp/WinApp.h"

ChoEngine::ChoEngine()
{
}

ChoEngine::~ChoEngine()
{
}

void ChoEngine::Initialize()
{
	// ResourceLeakChecker
	resourceLeakChecker = std::make_unique<ResourceLeakChecker>();

	// ウィンドウの作成
	WinApp::CreateGameWindow();

	// DirectX12初期化
	dx12 = std::make_unique<DirectX12Common>();

	// CommandManager初期化
	commandManager = std::make_unique<CommandManager>(dx12->GetDevice());

	// PlatformLayer初期化
	platformLayer = std::make_unique<PlatformLayer>();

	// CoreSystem初期化
	coreSystem = std::make_unique<CoreSystem>();

	// ResourceManager初期化
	resourceManager = std::make_unique<ResourceManager>();

	// SwapChain初期化
	swapChain = std::make_unique<SwapChain>(
		dx12->GetDXGIFactory(),
		commandManager->GetCommandQueue(QueueType::Graphics),
		WinApp::GetHWND(),
		WinApp::GetWindowWidth(),
		WinApp::GetWindowHeight()
	);
}

void ChoEngine::Finalize()
{
	// PlatformLayer終了処理
	platformLayer->Finalize();
	// DirectX12終了処理
	dx12->Finalize();
	// ウィンドウの破棄
	WinApp::TerminateWindow();
}

void ChoEngine::Operation()
{
	/*初期化*/
	Initialize();

	/*メインループ*/
	while (true) {
		if (WinApp::ProcessMessage()) {
			break;
		}
		// 開始
		Start();
		// 更新
		Update();
		// 終了
		End();
	}

	/*終了処理*/
	Finalize();
}

void ChoEngine::Update()
{
	// PlatformLayer更新
	platformLayer->Update();
}

void ChoEngine::Start()
{
	// PlatformLayer記録開始
	platformLayer->StartFrame();
}

void ChoEngine::End()
{
	// PlatformLayer記録終了
	platformLayer->EndFrame();
}
