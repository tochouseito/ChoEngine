#include "pch.h"
#include "include/Main/Engine.h"

// === Engine Systemes ===
#include "include/Platform/FileSystem.h"
#include "include/Platform/Input.h"
#include "include/Platform/Network.h"
#include "include/Platform/Thread.h"
#include "include/Platform/Timer.h"
#include "include/Platform/WinApp.h"
#include "include/Core/Allocators.h"
#include "include/Core/CommandSystem.h"
#include "include/Core/EventSystem.h"
#include "include/Core/FileController.h"
#include "include/Core/JobSystem.h"
#include "include/Core/LogAssert.h"
#include "include/Core/MemoryManager.h"
#include "include/Core/UUID.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/FrameGraph.h"
#include "include/Graphics/GPUTimeline.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/ShaderCompiler.h"
#include "include/Physics/PhysicsWorld.h"
#include "include/Audio/AudioEngine.h"
#include "include/GameCore/SceneManager.h"
#include "include/Assets/IDManager.h"
#include "include/Assets/Loader.h"
#include "include/Scripting/ScriptAPI.h"

// === Editor ===
#include "include/Editor/ImGuiManager.h"

using namespace Theatria;

/// @brief 実装隠蔽クラス
class Engine::Impl
{
public:
    Impl()
    {
        m_pFileSystem = std::make_unique<Platform::FileSystem>();
        m_pInput = std::make_unique<Platform::Input>();
        m_pNetwork = std::make_unique<Platform::Network>();
        m_pThread = std::make_unique<Platform::Thread>();
        m_pTimer = std::make_unique<Platform::Timer>();
        m_pWinApp = std::make_unique<Platform::WinApp>();
        m_pAllocators = std::make_unique<Core::Allocators>();
        m_pCommandSystem = std::make_unique<Core::CommandSystem>();
        m_pEventSystem = std::make_unique<Core::EventSystem>();
        m_pFileController = std::make_unique<Core::FileController>();
        m_pJobSystem = std::make_unique<Core::JobSystem>();
        m_pLogAssert = std::make_unique<Core::LogAssert>();
        m_pMemoryManager = std::make_unique<Core::MemoryManager>();
        m_pUUID = std::make_unique<Core::UUID>();
        m_pDescriptorAllocator = std::make_unique<Graphics::DescriptorAllocator>();
        m_pFrameGraph = std::make_unique<Graphics::FrameGraph>();
        m_pGPUTimeline = std::make_unique<Graphics::GPUTimeline>();
        m_pRenderDevice = std::make_unique<Graphics::RenderDevice>();
        m_pRenderer = std::make_unique<Graphics::Renderer>();
        m_pResourceManager = std::make_unique<Graphics::ResourceManager>();
        m_pShaderCompiler = std::make_unique<Graphics::ShaderCompiler>();
        m_pPhysicsWorld = std::make_unique<Physics::PhysicsWorld>();
        m_pAudioEngine = std::make_unique<Audio::AudioEngine>();
        m_pSceneManager = std::make_unique<GameCore::SceneManager>();
        m_pIDManager = std::make_unique<Assets::IDManager>();
        m_pLoader = std::make_unique<Assets::Loader>();
        m_pScriptAPI = std::make_unique<Scripting::ScriptAPI>();

        m_pImGuiManager = std::make_unique<Editor::ImGuiManager>();

    }
    ~Impl() noexcept
    {
        m_pImGuiManager.reset();
        m_pScriptAPI.reset();
        m_pLoader.reset();
        m_pIDManager.reset();
        m_pSceneManager.reset();
        m_pAudioEngine.reset();
        m_pPhysicsWorld.reset();
        m_pShaderCompiler.reset();
        m_pResourceManager.reset();
        m_pRenderer.reset();
        m_pRenderDevice.reset();
        m_pGPUTimeline.reset();
        m_pFrameGraph.reset();
        m_pDescriptorAllocator.reset();
        m_pUUID.reset();
        m_pMemoryManager.reset();
        m_pLogAssert.reset();
        m_pJobSystem.reset();
        m_pFileController.reset();
        m_pEventSystem.reset();
        m_pCommandSystem.reset();
        m_pAllocators.reset();
        m_pWinApp.reset();
        m_pTimer.reset();
        m_pThread.reset();
        m_pNetwork.reset();
        m_pInput.reset();
        m_pFileSystem.reset();
    }
private:
    std::unique_ptr<Platform::FileSystem>           m_pFileSystem;           ///< ファイルシステム
    std::unique_ptr<Platform::Input>                m_pInput;                ///< 入力システム
    std::unique_ptr<Platform::Network>              m_pNetwork;              ///< ネットワークシステム
    std::unique_ptr<Platform::Thread>               m_pThread;               ///< スレッドシステム
    std::unique_ptr<Platform::Timer>                m_pTimer;                ///< タイマーシステム
    std::unique_ptr<Platform::WinApp>               m_pWinApp;               ///< Windowsアプリケーション
    std::unique_ptr<Core::Allocators>               m_pAllocators;           ///< アロケータシステム
    std::unique_ptr<Core::CommandSystem>            m_pCommandSystem;        ///< コマンドシステム
    std::unique_ptr<Core::EventSystem>              m_pEventSystem;          ///< イベントシステム
    std::unique_ptr<Core::FileController>           m_pFileController;       //< ファイルコントローラ
    std::unique_ptr<Core::JobSystem>                m_pJobSystem;            ///< ジョブシステム
    std::unique_ptr<Core::LogAssert>                m_pLogAssert;            ///< ログアサートシステム
    std::unique_ptr<Core::MemoryManager>            m_pMemoryManager;        ///< メモリマネージャ
    std::unique_ptr<Core::UUID>                     m_pUUID;                 ///< UUID生成システム
    std::unique_ptr<Graphics::DescriptorAllocator>  m_pDescriptorAllocator;  ///< ディスクリプタアロケータ
    std::unique_ptr<Graphics::FrameGraph>           m_pFrameGraph;           ///< フレームグラフ
    std::unique_ptr<Graphics::GPUTimeline>          m_pGPUTimeline;          ///< GPUタイムライン
    std::unique_ptr<Graphics::RenderDevice>         m_pRenderDevice;         ///< レンダーデバイス
    std::unique_ptr<Graphics::Renderer>             m_pRenderer;             ///< レンダラー
    std::unique_ptr<Graphics::ResourceManager>      m_pResourceManager;      ///< リソースマネージャ
    std::unique_ptr<Graphics::ShaderCompiler>       m_pShaderCompiler;       ///< シェーダーコンパイラ
    std::unique_ptr<Physics::PhysicsWorld>          m_pPhysicsWorld;         ///< 物理ワールド
    std::unique_ptr<Audio::AudioEngine>             m_pAudioEngine;          ///< オーディオエンジン
    std::unique_ptr<GameCore::SceneManager>         m_pSceneManager;         ///< シーンマネージャ
    std::unique_ptr<Assets::IDManager>              m_pIDManager;            ///< IDマネージャ
    std::unique_ptr<Assets::Loader>                 m_pLoader;               ///< アセットローダー
    std::unique_ptr<Scripting::ScriptAPI>           m_pScriptAPI;            ///< スクリプトAPI

    std::unique_ptr<Editor::ImGuiManager>           m_pImGuiManager;         ///< ImGuiマネージャ
};

/// @brief コンストラクタ
Theatria::Engine::Engine()
    : m_pImpl(std::make_unique<Impl>())
{
    // COM初期化
    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    hr;
    m_Run = Initialize();
}

/// @brief デストラクタ
Theatria::Engine::~Engine() noexcept
{
    Shutdown();
    // COM終了処理
    CoUninitialize();
}

/// @brief 稼働処理
void Theatria::Engine::Operation()
{
    while (m_Run)
    {

    }
}

/// @brief エンジン初期化 戻り値無視禁止
/// @return 初期化成功ならtrue、失敗ならfalse
[[nodiscard]]
bool Theatria::Engine::Initialize()
{
    return true;
}

/// @brief エンジン終了処理
void Theatria::Engine::Shutdown()
{

}
