#pragma once

/*--------------------------------------------* 

TheatriaEngineクラス

--------------------------------------------*/

#include "Engine/Engine.h"
// Windows
#include "OS/Windows/WinApp/WinApp.h"
// DirectX12
#include "SDK/DirectX/DirectX12/DirectX12Common/DirectX12Common.h"
// PlatformLayer
#include "Platform/PlatformLayer/PlatformLayer.h"
// CoreSystem
#include "Core/CoreSystem/CoreSystem.h"
// Resource
#include "Resources/ResourceManager/ResourceManager.h"
// GraphicsEngine
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
// ImGuiManager
#include "SDK/ImGui/ImGuiManager/ImGuiManager.h"
// GameCore
#include "GameCore/GameCore.h"
// EngineCommand
#include "EngineCommand/EngineCommand.h"
// EditorManager
#include "Editor/EditorManager/EditorManager.h"
// HubManager
#include "Hub/HubManager.h"
// Config
#include "Config/Config.h"

class TheatriaEngine : public Engine
{
public:// method
	// コンストラクタ
	TheatriaEngine(RuntimeMode mode);
	// デストラクタ
	~TheatriaEngine();
	// 初期化
	void Initialize() override;
	// 終了処理
	void Finalize() override;
	// 稼働
	void Operation() override;
	// 更新
	void Update();
	// 描画
	void Draw();
	// 開始
	void Start();
	// 終了
	void End();
	// クラッシュ時の処理
	static void OnCrashHandler();
	void CrashHandlerEntry() override;

	EngineCommand* GetEngineCommand() { return engineCommand.get(); }
private:// member
	// DirectX12
	std::unique_ptr<ResourceLeakChecker> resourceLeakChecker = nullptr;// ResourceLeakChecker
	std::unique_ptr<DirectX12Common> dx12 = nullptr;// DirectX12Common
	// PlatformLayer
	std::unique_ptr<PlatformLayer> platformLayer = nullptr;// PlatformLayer
	// CoreSystem
	std::unique_ptr<CoreSystem> coreSystem = nullptr;// CoreSystem
	// ResourceManager
	std::unique_ptr<ResourceManager> resourceManager = nullptr;// ResourceManager
	// GraphicsEngine
	std::unique_ptr<GraphicsEngine> graphicsEngine = nullptr;// GraphicsEngine
	// ImGuiManager
	std::unique_ptr<ImGuiManager> imGuiManager = nullptr;// ImGuiManager
	// GameCore
	std::unique_ptr<GameCore> gameCore = nullptr;// GameCore
	// EngineCommand
	std::unique_ptr<EngineCommand> engineCommand = nullptr;// EngineCommand
	// EditorManager
	std::unique_ptr<EditorManager> editorManager = nullptr;// EditorManager
	// HubManager
	std::unique_ptr<HubManager> hubManager = nullptr;// HubManager
    // Config
    std::unique_ptr<theatria::Config> config = nullptr;
};

