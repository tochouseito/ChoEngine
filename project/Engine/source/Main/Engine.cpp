#include "pch.h"
#include "include/Main/Engine.h"

// === Engine Systemes ===
#include "include/Platform/FileSystem.h"
#include "include/Platform/Input.h"
#include "include/Platform/Network.h"
#include "include/Platform/Thread.h"
#include "include/Platform/Timer.h"
#include "include/Platform/WinApp.h"
#include "include/Core/FrameCounter.h"
#include "include/Core/Allocators.h"
#include "include/Core/EventCommand.h"
#include "include/Core/FileController.h"
#include "include/Core/JobSystem.h"
#include "include/Core/LogAssert.h"
#include "include/Core/MemoryManager.h"
#include "include/Core/UUID.h"
#include "include/Graphics/ResourceLeakChecker.h"
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

// === EventCommands ===
#include "include/Core/Events.h"
#include "include/Core/Commands.h"
#include "include/Core/Routers.h"

// === Editor ===
#include "include/Editor/ImGuiManager.h"

using namespace Theatria;

/// @brief 実装隠蔽クラス
class Engine::Impl
{
    friend class Engine;
public:
    Impl()
    {
        /*======================== EventCommand ========================*/
        m_pEventSystem = std::make_unique<Core::EventCommand::EventSystem>();
        m_pRouterHub = std::make_unique<Core::EventCommand::RouterHub>();
        m_pExecutorHub = std::make_unique<Core::EventCommand::ExecutorHub>();
        /*======================== Platform ========================*/
        m_pInput =                  std::make_unique<Platform::Input>();
        m_pNetwork =                std::make_unique<Platform::Network>();
        /*======================== Core ========================*/
        m_pFrameCounter =           std::make_unique<Core::FrameCounter>();
        m_pAllocators =             std::make_unique<Core::Allocators>();
        m_pFileController =         std::make_unique<Core::FileController>();
        m_pJobSystem =              std::make_unique<Core::JobSystem>();
        m_pMemoryManager =          std::make_unique<Core::MemoryManager>();
        m_pUUID =                   std::make_unique<Core::UUID>();
        /*======================== Graphics ========================*/
        m_pResourceLeakChecker =    std::make_unique<Graphics::ResourceLeakChecker>();
        m_pShaderCompiler =         std::make_unique<Graphics::ShaderCompiler>();
        m_pRenderDevice =           std::make_unique<Graphics::RenderDevice>();
        m_pDescriptorAllocator =    std::make_unique<Graphics::DescriptorAllocator>();
        m_pResourceManager =        std::make_unique<Graphics::ResourceManager>();
        m_pGPUTimeline =            std::make_unique<Graphics::GPUTimeline>();
        m_pFrameGraph =             std::make_unique<Graphics::FrameGraph>();
        m_pRenderer =               std::make_unique<Graphics::Renderer>();
        /*======================== Other Systems ========================*/
        m_pPhysicsWorld =           std::make_unique<Physics::PhysicsWorld>();
        m_pAudioEngine =            std::make_unique<Audio::AudioEngine>();
        m_pSceneManager =           std::make_unique<GameCore::SceneManager>();
        m_pIDManager =              std::make_unique<Assets::IDManager>();
        m_pLoader =                 std::make_unique<Assets::Loader>();
        m_pScriptAPI =              std::make_unique<Scripting::ScriptAPI>();

        m_pImGuiManager =           std::make_unique<Editor::ImGuiManager>();

    }
    ~Impl() noexcept
    {
        /*======================== Other Systems ========================*/
        m_pImGuiManager.reset();
        m_pScriptAPI.reset();
        m_pLoader.reset();
        m_pIDManager.reset();
        m_pSceneManager.reset();
        m_pAudioEngine.reset();
        m_pPhysicsWorld.reset();
        /*======================== Graphics ========================*/
        m_pRenderer.reset();
        m_pFrameGraph.reset();
        m_pGPUTimeline.reset();
        m_pResourceManager.reset();
        m_pDescriptorAllocator.reset();
        m_pRenderDevice.reset();
        m_pShaderCompiler.reset();
        m_pResourceLeakChecker.reset();
        /*======================== Core ========================*/
        m_pUUID.reset();
        m_pMemoryManager.reset();
        m_pJobSystem.reset();
        m_pFileController.reset();
        m_pAllocators.reset();
        m_pFrameCounter.reset();
        /*======================== Platform ========================*/
        m_pNetwork.reset();
        m_pInput.reset();
        /*======================== EventCommand ========================*/
        m_pExecutorHub.reset();
        m_pRouterHub.reset();
        m_pEventSystem.reset();
    }
private:
    /*======================== EventCommand ========================*/
    std::unique_ptr<Core::EventCommand::EventSystem>  m_pEventSystem;          ///< イベントシステム
    std::unique_ptr<Core::EventCommand::RouterHub>    m_pRouterHub;            ///< ルーターハブ
    std::unique_ptr<Core::EventCommand::ExecutorHub>  m_pExecutorHub;          ///< エグゼキューターハブ
    /*======================== Platform ========================*/
    std::unique_ptr<Platform::Input>                m_pInput;                ///< 入力システム
    std::unique_ptr<Platform::Network>              m_pNetwork;              ///< ネットワーク
    /*======================== Core ========================*/
    std::unique_ptr<Core::FrameCounter>             m_pFrameCounter;         ///< フレームカウンタ
    std::unique_ptr<Core::Allocators>               m_pAllocators;           ///< アロケータシステム
    std::unique_ptr<Core::FileController>           m_pFileController;       ///< ファイルコントローラ
    std::unique_ptr<Core::JobSystem>                m_pJobSystem;            ///< ジョブシステム
    std::unique_ptr<Core::MemoryManager>            m_pMemoryManager;        ///< メモリマネージャ
    std::unique_ptr<Core::UUID>                     m_pUUID;                 ///< UUID生成システム
    /*======================== Graphics ========================*/
    std::unique_ptr<Graphics::ResourceLeakChecker>  m_pResourceLeakChecker;  ///< リソースリークチェッカー
    std::unique_ptr<Graphics::ShaderCompiler>       m_pShaderCompiler;       ///< シェーダーコンパイラ
    std::unique_ptr<Graphics::RenderDevice>         m_pRenderDevice;         ///< レンダーデバイス
    std::unique_ptr<Graphics::DescriptorAllocator>  m_pDescriptorAllocator;  ///< ディスクリプタアロケータ
    std::unique_ptr<Graphics::ResourceManager>      m_pResourceManager;      ///< リソースマネージャ
    std::unique_ptr<Graphics::GPUTimeline>          m_pGPUTimeline;          ///< GPUタイムライン
    std::unique_ptr<Graphics::FrameGraph>           m_pFrameGraph;           ///< フレームグラフ
    std::unique_ptr<Graphics::Renderer>             m_pRenderer;             ///< レンダラー
    /*======================== Other Systems ========================*/
    std::unique_ptr<Physics::PhysicsWorld>          m_pPhysicsWorld;         ///< 物理ワールド
    std::unique_ptr<Audio::AudioEngine>             m_pAudioEngine;          ///< オーディオエンジン
    std::unique_ptr<GameCore::SceneManager>         m_pSceneManager;         ///< シーンマネージャ
    std::unique_ptr<Assets::IDManager>              m_pIDManager;            ///< IDマネージャ
    std::unique_ptr<Assets::Loader>                 m_pLoader;               ///< アセットローダー
    std::unique_ptr<Scripting::ScriptAPI>           m_pScriptAPI;            ///< スクリプトAPI

    std::unique_ptr<Editor::ImGuiManager>           m_pImGuiManager;         ///< ImGuiマネージャ

    Core::EventCommand::CommandBuffer m_WinAppQuere;    ///< WinAppコマンドキュー
};

/// @brief コンストラクタ
Theatria::Engine::Engine(RuntimeMode mode)
    : m_pImpl(std::make_unique<Impl>()), m_RuntimeMode(mode)
{
    
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
    // COM初期化
    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (!Core::LogAssert::Verify(hr, "COM Initialize", "COM InitializeEx failed"))
    {
        m_Run = false;
        return;
    }
    // エンジン初期化
    m_Run = Initialize();

    while (m_Run)
    {
        // フレーム開始
        m_pImpl->m_pFrameCounter->BeginFrame();
        if (Platform::WinApp::ProcessMessage())
        {
            m_Run = false;
            break;
        }

        // ルーターがイベントを受けて、コマンドをQuereに積む
        m_pImpl->m_pRouterHub->FlushAll();
        // コマンドを実行
        m_pImpl->m_pExecutorHub->ExecuteAll();
        // フレームスリープ
        m_pImpl->m_pFrameCounter->SleepFrame();
    }

    // エンジン終了処理
    Shutdown();
}

/// @brief エンジン初期化 戻り値無視禁止
/// @return 初期化成功ならtrue、失敗ならfalse
[[nodiscard]]
bool Theatria::Engine::Initialize()
{
    /*======================== 各種システム初期化処理 ========================*/

    // EventCommand 作成
    /*m_pImpl->m_pRouterHub->Add<Core::Routers::ShowWindowRouter>(*m_pImpl->m_pEventSystem.get(), m_pImpl->m_WinAppQuere);
    m_pImpl->m_pExecutorHub->Register(m_pImpl->m_WinAppQuere, [&](Core::EventCommand::CommandBuffer& q) {
        q.ExecuteAll(nullptr);
        });*/

    // ウィンドウの作成
    Platform::WinApp::CreateWindowApp();
    // ウィンドウの表示
    Platform::WinApp::ShowWindowApp();
    // m_pImpl->m_pEventSystem->Publish(Core::Events::EveShowWindow{ "Theatria Engine Window" });

    /*======================== Input ========================*/
    /*
    入力処理の初期化
    */

    /*======================== Core ========================*/
    m_pImpl->m_pJobSystem->Initialize();

    /*======================== Graphics ========================*/
    // レンダーデバイス初期化
    if (!m_pImpl->m_pRenderDevice->Initialize(true))
    {
        Core::LogAssert::Verify(false, "RenderDevice Initialize", "RenderDevice initialization failed");
        return false;
    }
    // ディスクリプタアロケータ初期化
    if (!m_pImpl->m_pDescriptorAllocator->Initialize(m_pImpl->m_pRenderDevice.get(), 2048, 2048))
    {
        Core::LogAssert::Verify(false, "DescriptorAllocator Initialize", "DescriptorAllocator initialization failed");
        return false;
    }
    // リソースマネージャ初期化
    if (!m_pImpl->m_pResourceManager->Initialize(m_pImpl->m_pRenderDevice.get(), m_pImpl->m_pDescriptorAllocator.get()))
    {
        Core::LogAssert::Verify(false, "ResourceManager Initialize", "ResourceManager initialization failed");
        return false;
    }
    // レンダラー初期化
    if (!m_pImpl->m_pRenderer->Initialize(m_pImpl->m_pRenderDevice.get(),m_pImpl->m_pResourceManager.get()))
    {
        Core::LogAssert::Verify(false, "Renderer Initialize", "Renderer initialization failed");
        return false;
    }
    // スワップチェーン作成
    if (!m_pImpl->m_pRenderDevice->CreateSwapChain(m_pImpl->m_pDescriptorAllocator.get()))
    {
        Core::LogAssert::Verify(false, "SwapChain Create", "SwapChain creation failed");
        return false;
    }

    return true;
}

/// @brief エンジン終了処理
void Theatria::Engine::Shutdown()
{
    // ジョブシステムのスレッド停止
    m_pImpl->m_pJobSystem->StopAllThreads();
}
