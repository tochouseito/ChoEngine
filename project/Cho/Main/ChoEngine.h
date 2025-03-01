#pragma once

/*--------------------------------------------
ChoEngineクラス
--------------------------------------------*/

#include "Cho/Engine/Engine.h"

// Windows
#include "Cho/OS/Windows/WinApp/WinApp.h"

// DirectX12
#include "Cho/SDK/DirectX/DirectX12/DirectX12Common/DirectX12Common.h"
#include "Cho/SDK/DirectX/DirectX12/CommandManager/CommandManager.h"
#include "Cho/SDK/DirectX/DirectX12/SwapChain/SwapChain.h"

// PlatformLayer
#include "Cho/Platform/PlatformLayer/PlatformLayer.h"

// CoreSystem
#include "Cho/Core/CoreSystem/CoreSystem.h"

// Resource
#include "Cho/Resources/ResourceManager/ResourceManager.h"

class ChoEngine : public Engine
{
public:// method

	// コンストラクタ
	ChoEngine();

	// デストラクタ
	~ChoEngine();

	// 初期化
	void Initialize() override;

	// 終了処理
	void Finalize() override;

	// 稼働
	void Operation() override;

	// 更新
	void Update();

	// 開始
	void Start();

	// 終了
	void End();
private:// member

	// DirectX12
	std::unique_ptr<ResourceLeakChecker> resourceLeakChecker = nullptr;// ResourceLeakChecker
	std::unique_ptr<DirectX12Common> dx12 = nullptr;// DirectX12Common
	std::unique_ptr<CommandManager> commandManager = nullptr;// CommandManager
	std::unique_ptr<SwapChain> swapChain = nullptr;// SwapChain

	// PlatformLayer
	std::unique_ptr<PlatformLayer> platformLayer = nullptr;// PlatformLayer

	// CoreSystem
	std::unique_ptr<CoreSystem> coreSystem = nullptr;// CoreSystem

	// ResourceManager
	std::unique_ptr<ResourceManager> resourceManager = nullptr;// ResourceManager
};

