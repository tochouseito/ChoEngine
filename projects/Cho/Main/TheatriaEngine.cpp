#include "pch.h"
#include "TheatriaEngine.h"
#include <exception>

// Windowアプリケーション
#include "OS/Windows/WinApp/WinApp.h"

TheatriaEngine::TheatriaEngine(RuntimeMode mode):
	Engine(mode)
{
}

TheatriaEngine::~TheatriaEngine()
{
}

void TheatriaEngine::Initialize()
{
	// ResourceLeakChecker
	resourceLeakChecker = std::make_unique<ResourceLeakChecker>();

    // Config
    config = std::make_unique<theatria::Config>();

	// ウィンドウの作成
	WinApp::CreateGameWindow();

	// DirectX12初期化
	dx12 = std::make_unique<DirectX12Common>();

	// PlatformLayer初期化
	platformLayer = std::make_unique<PlatformLayer>();

	// CoreSystem初期化
	coreSystem = std::make_unique<CoreSystem>();

	// ResourceManager初期化
	resourceManager = std::make_unique<ResourceManager>(dx12->GetDevice());

	// GraphicsEngine初期化
	graphicsEngine = std::make_unique<GraphicsEngine>(dx12->GetDevice(),resourceManager.get(),GetRuntimeMode(), coreSystem->GetThreadManager());
	graphicsEngine->CreateSwapChain(dx12->GetDXGIFactory());
	graphicsEngine->Init();

	// Model,TextureManager初期化
	resourceManager->GenerateManager(graphicsEngine.get());

	// ImGuiManager初期化
	imGuiManager = std::make_unique<ImGuiManager>();
	imGuiManager->Initialize(dx12->GetDevice(), resourceManager.get());

	// GameCore初期化
	gameCore = std::make_unique<GameCore>();
	gameCore->Initialize(resourceManager.get(),graphicsEngine.get());

	// EngineCommand初期化
	engineCommand = std::make_unique<EngineCommand>(gameCore.get(), resourceManager.get(), graphicsEngine.get(),platformLayer->GetInputManager(),platformLayer.get(),config.get());
	// GameCoreにEngineCommandをセット
	gameCore->SetEngineCommandPtr(engineCommand.get());
	graphicsEngine->SetEngineCommand(engineCommand.get());

	// EditorManager初期化
	editorManager = std::make_unique<EditorManager>(engineCommand.get(), platformLayer->GetInputManager());
	editorManager->Initialize(runtimeMode == RuntimeMode::Game);
    gameCore->SetScriptSystemEngineCommand();

	// HubManager初期化
	hubManager = std::make_unique<HubManager>(platformLayer.get(), coreSystem.get(), engineCommand.get(), runtimeMode == RuntimeMode::Game ? true : false);
	hubManager->Initialize();
}

void TheatriaEngine::Finalize()
{
	// GPUの完了待ち
	graphicsEngine->Finalize();
	// ファイルの保存
	if (runtimeMode == RuntimeMode::Editor)
	{
		// ゲーム実行中なら終了する
		if (gameCore->IsRunning())
		{
			gameCore->GameStop();
			editorManager->ReloadEditingScene();
		}
		FileSystem::SaveProject(editorManager.get(), gameCore->GetSceneManager(), gameCore->GetGameWorld(), gameCore->GetECSManager(),engineCommand.get());
	}
    // GameCore終了処理
    gameCore->Finalize();
	// ImGuiManager終了処理
	imGuiManager->Finalize();
	// ResourceManager終了処理
	resourceManager->Finalize();
	// PlatformLayer終了処理
	platformLayer->Finalize();
	// DirectX12終了処理
	dx12->Finalize();
	// スレッドクローズ
	coreSystem->Finalize();
	// ウィンドウの破棄
	WinApp::TerminateWindow();
}

void TheatriaEngine::Operation()
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
		// 描画
		Draw();
		// 終了
		End();
	}

	/*終了処理*/
	Finalize();
}

void TheatriaEngine::Update()
{
	// PlatformLayer更新
	platformLayer->Update();
	// ImGuiManager開始
	imGuiManager->Begin();
	if (hubManager->IsRun())
	{
		// HubManager更新
		hubManager->Update();
	} else
	{
		if (runtimeMode == RuntimeMode::Editor)
		{
			// EditorManager更新
			editorManager->Update();
		}
	}
	//gameCore->SceneUpdate();
	static bool isGameRun = true;
	if (runtimeMode == RuntimeMode::Game && isGameRun)
	{
		gameCore->GameRun(false,true);
		isGameRun = false;
	}
	// GameCore更新
	gameCore->Update();
	// ImGuiManager終了
	imGuiManager->End();
}

void TheatriaEngine::Draw()
{
	//　描画準備
	graphicsEngine->PreRender();
	if (runtimeMode == RuntimeMode::Editor)
	{
		// 描画
		graphicsEngine->Render(*resourceManager, *gameCore, RenderMode::Game);
		// シーン描画
		graphicsEngine->Render(*resourceManager, *gameCore, RenderMode::Scene);
		// 描画後片付け
		graphicsEngine->PostRender(imGuiManager.get(), RenderMode::Game);
	}
	if (runtimeMode == RuntimeMode::Game)
	{
		// 描画
		graphicsEngine->Render(*resourceManager, *gameCore, RenderMode::Release);
		// 描画後片付け
		graphicsEngine->PostRender(imGuiManager.get(), RenderMode::Release);
	}
}

void TheatriaEngine::Start()
{
	// PlatformLayer記録開始
	platformLayer->StartFrame();
	// ウィンドウのリサイズ処理
	if (WinApp::IsResizeWindow())
	{
		graphicsEngine->ScreenResize();
	}
	// ウィンドウのフォーカスが外れたらプロジェクト保存
	if (WinApp::IsKillfocusWindow()&&!hubManager->IsRun())
	{
		if (runtimeMode == RuntimeMode::Editor&&!gameCore->IsRunning())
		{
			FileSystem::SaveProject(editorManager.get(), gameCore->GetSceneManager(), gameCore->GetGameWorld(), gameCore->GetECSManager(),engineCommand.get());
		}
	}
	// ブランチが変更されたかどうか
	if (hubManager->CheckBranchChanged())
	{
		// ブランチが変更されたらプロジェクト再読み込み
		// システム、エンジンをリセット、初期化
		graphicsEngine->Finalize();
		imGuiManager->Finalize();
		hubManager.reset();
		editorManager.reset();
		engineCommand.reset();
		gameCore.reset();
		imGuiManager.reset();
		graphicsEngine.reset();
		resourceManager.reset();
		// 再生成
		// ResourceManager初期化
		resourceManager = std::make_unique<ResourceManager>(dx12->GetDevice());
		// GraphicsEngine初期化
		graphicsEngine = std::make_unique<GraphicsEngine>(dx12->GetDevice(), resourceManager.get(), GetRuntimeMode(), coreSystem->GetThreadManager());
		graphicsEngine->CreateSwapChain(dx12->GetDXGIFactory());
		graphicsEngine->Init();
		// Model,TextureManager初期化
		resourceManager->GenerateManager(graphicsEngine.get());
		// ImGuiManager初期化
		imGuiManager = std::make_unique<ImGuiManager>();
		imGuiManager->Initialize(dx12->GetDevice(), resourceManager.get());
		// GameCore初期化
		gameCore = std::make_unique<GameCore>();
		gameCore->Initialize(resourceManager.get(),graphicsEngine.get());
		// EngineCommand初期化
		engineCommand = std::make_unique<EngineCommand>(gameCore.get(), resourceManager.get(), graphicsEngine.get(),platformLayer->GetInputManager(), platformLayer.get(), config.get());
		// GameCoreにEngineCommandをセット
		gameCore->SetEngineCommandPtr(engineCommand.get());
		graphicsEngine->SetEngineCommand(engineCommand.get());
		
		// EditorManager初期化
		editorManager = std::make_unique<EditorManager>(engineCommand.get(), platformLayer->GetInputManager());
		editorManager->Initialize();
		// HubManager初期化
		hubManager = std::make_unique<HubManager>(platformLayer.get(), coreSystem.get(), engineCommand.get(), runtimeMode == RuntimeMode::Game ? true : false);
		hubManager->Initialize();
		// プロジェクト読み込み
		hubManager->ReloadProject();

	}
	// GameCore開始
	//gameCore->Start(*resourceManager);
}

void TheatriaEngine::End()
{
	// PlatformLayer記録終了
	platformLayer->EndFrame();
}

void TheatriaEngine::OnCrashHandler()
{
	// クラッシュ時の処理
}

void TheatriaEngine::CrashHandlerEntry()
{
	std::set_terminate(OnCrashHandler);
}
