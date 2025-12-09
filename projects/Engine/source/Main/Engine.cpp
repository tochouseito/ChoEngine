#include "pch.h"
#include "include/Main/Engine.h"
#include "config/engineConfig.h"

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
#include "include/Core/FrameJob.h"
#include "include/Graphics/ResourceLeakChecker.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/FrameGraph.h"
#include "include/Graphics/GPUTimeline.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/ShaderCompiler.h"
#include "include/Graphics/PipelineManager.h"
#include "include/Physics/PhysicsWorld.h"
#include "include/Audio/AudioEngine.h"
#include "include/GameCore/SceneManager.h"
#include "include/GameCore/ECSManager.h"
#include "include/GameCore/GameWorld.h"
#include "include/Assets/IDManager.h"
#include "include/Assets/Loader.h"
#include "include/Assets/ModelContainer.h"
#include "include/Scripting/ScriptAPI.h"

// === EventCommands ===
#include "include/Core/Events.h"
#include "include/Core/Commands.h"
#include "include/Core/Routers.h"

// === Editor ===
#include "include/Editor/ImGuiManager.h"
#include "include/Editor/EditorManager.h"

using namespace Theatria;

/// @brief 実装隠蔽クラス
class Engine::Impl
{
    friend class Engine;
public:
    Impl()
    {
        /*======================== EventCommand ========================*/
        m_pEventSystem =            std::make_unique<Core::EventCommand::EventSystem>();
        m_pRouterHub =              std::make_unique<Core::EventCommand::RouterHub>();
        m_pExecutorHub =            std::make_unique<Core::EventCommand::ExecutorHub>();
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
        m_pPipelineManager =        std::make_unique<Graphics::PipelineManager>();
        m_pRenderDevice =           std::make_unique<Graphics::RenderDevice>();
        m_pDescriptorAllocator =    std::make_unique<Graphics::DescriptorAllocator>();
        m_pResourceManager =        std::make_unique<Graphics::ResourceManager>();
        m_pGPUTimeline =            std::make_unique<Graphics::GPUTimeline>();
        m_pFrameGraph =             std::make_unique<Graphics::FrameGraph>();
        m_pRenderer =               std::make_unique<Graphics::Renderer>();
        /*======================== Other Systems ========================*/
        m_pPhysicsWorld =           std::make_unique<Physics::PhysicsWorld>();
        m_pAudioEngine =            std::make_unique<Audio::AudioEngine>();
        m_pIDManager =              std::make_unique<Assets::IDManager>();
        m_pLoader =                 std::make_unique<Assets::Loader>();
        m_pScriptAPI =              std::make_unique<Scripting::ScriptAPI>();
        /*======================== GameCore ========================*/
        m_pECSManager =             std::make_unique<GameCore::ECSManager>();
        m_pGameWorld =              std::make_unique<GameCore::GameWorld>();
        m_pSceneManager =           std::make_unique<GameCore::SceneManager>();
        /*======================== Assets ========================*/
        m_pModelContainer =         std::make_unique<Assets::ModelContainer>();
#ifndef NDEBUG
        /*======================== Editor ========================*/
        m_pImGuiManager =           std::make_unique<Editor::ImGuiManager>();
        m_pEditorManager =          std::make_unique<Editor::EditorManager>();
#endif // !NDEBUG

    }
    ~Impl() noexcept
    {
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
    std::unique_ptr<Graphics::PipelineManager>      m_pPipelineManager;      ///< パイプラインマネージャ
    std::unique_ptr<Graphics::RenderDevice>         m_pRenderDevice;         ///< レンダーデバイス
    std::unique_ptr<Graphics::DescriptorAllocator>  m_pDescriptorAllocator;  ///< ディスクリプタアロケータ
    std::unique_ptr<Graphics::ResourceManager>      m_pResourceManager;      ///< リソースマネージャ
    std::unique_ptr<Graphics::GPUTimeline>          m_pGPUTimeline;          ///< GPUタイムライン
    std::unique_ptr<Graphics::FrameGraph>           m_pFrameGraph;           ///< フレームグラフ
    std::unique_ptr<Graphics::Renderer>             m_pRenderer;             ///< レンダラー
    /*======================== Other Systems ========================*/
    std::unique_ptr<Physics::PhysicsWorld>          m_pPhysicsWorld;         ///< 物理ワールド
    std::unique_ptr<Audio::AudioEngine>             m_pAudioEngine;          ///< オーディオエンジン
    std::unique_ptr<Assets::IDManager>              m_pIDManager;            ///< IDマネージャ
    std::unique_ptr<Assets::Loader>                 m_pLoader;               ///< アセットローダー
    std::unique_ptr<Scripting::ScriptAPI>           m_pScriptAPI;            ///< スクリプトAPI
    /*======================== GameCore ========================*/
    std::unique_ptr<GameCore::ECSManager>           m_pECSManager;           ///< ECSマネージャ
    std::unique_ptr<GameCore::GameWorld>            m_pGameWorld;            ///< ゲームワールド
    std::unique_ptr<GameCore::SceneManager>         m_pSceneManager;         ///< シーンマネージャ
    /*======================== Assets ========================*/
    std::unique_ptr<Assets::ModelContainer>        m_pModelContainer;       ///< モデルコンテナ
#ifndef NDEBUG
    /*======================== Editor ========================*/
    std::unique_ptr<Editor::ImGuiManager>           m_pImGuiManager;         ///< ImGuiマネージャ
    std::unique_ptr<Editor::EditorManager>        m_pEditorManager;        ///< エディタマネージャ
#endif // !NDEBUG

    Core::EventCommand::CommandBuffer m_WinAppQuere;    ///< WinAppコマンドキュー

    // フレームジョブ
    Core::FrameJob updateJob;
    Core::FrameJob renderJob;
};

/// @brief コンストラクタ
Theatria::Engine::Engine(RuntimeMode mode)
    : m_pImpl(std::make_unique<Impl>()), m_RuntimeMode(mode)
{
    // COM初期化
    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (!Core::LogAssert::Verify(hr, "COM Initialize", "COM InitializeEx failed"))
    {
        m_Run = false;
        return;
    }
}

/// @brief デストラクタ
Theatria::Engine::~Engine() noexcept
{
    // COM終了処理
    CoUninitialize();
}

/// @brief 稼働処理
void Theatria::Engine::Operation()
{
    // エンジン初期化
    m_Run = Initialize();

    while (m_Run)
    {
        if (Platform::WinApp::ProcessMessage())
        {
            m_Run = false;
            break;
        }

        // ==== 0) 前フレームまでのコマンドが残っているなら全部処理 ====
        if (m_pImpl->m_pExecutorHub->HasPendingCommands())
        {
            // すでにキック済みのフレームが全部終わるまで待つ
            // Update/Render用のFrameJobがあるので、それのFinishedFrameを見る
            if (m_pImpl->m_pFrameCounter->m_TotalFrames < m_pImpl->m_pFrameCounter->m_ProduceFrame &&
                m_pImpl->updateJob.m_FinishedFrame >= m_pImpl->m_pFrameCounter->m_ProduceFrame - 1 &&
                m_pImpl->renderJob.m_FinishedFrame >= m_pImpl->m_pFrameCounter->m_ProduceFrame - 1)
            {
                // 最後のぶん Present してから止める
                m_pImpl->m_pRenderer->Present();
                // FPS計測+Sleep制御
                m_pImpl->m_pFrameCounter->Tick();
                // コマンドを実行
                m_pImpl->m_pExecutorHub->ExecuteAll();
                for (uint32_t i = 0; i < Config::Graphics::BufferingCount; ++i)
                {
                    Update(i); // 新レイアウト / 新サイズで全部埋め直す
                }
                continue;
            }
        }

        // ==== 1) 先行上限まで新しいフレームをキック ====
        if (m_pImpl->m_pFrameCounter->m_ProduceFrame - m_pImpl->m_pFrameCounter->m_TotalFrames < m_pImpl->m_pFrameCounter->GetMaxLead())
        {
            // このフレーム番号に対応するインデックスを計算
            const uint32_t presentIndex = static_cast<uint32_t>(m_pImpl->m_pFrameCounter->m_ProduceFrame % Config::Graphics::BufferingCount);
            const uint32_t renderIndex = (presentIndex + Config::Graphics::BufferingCount - 2) % Config::Graphics::BufferingCount;
            const uint32_t updateIndex = (presentIndex + Config::Graphics::BufferingCount - 1) % Config::Graphics::BufferingCount;

            m_pImpl->updateJob.Kick(m_pImpl->m_pFrameCounter->m_ProduceFrame, updateIndex);
            m_pImpl->renderJob.Kick(m_pImpl->m_pFrameCounter->m_ProduceFrame, renderIndex);

            ++m_pImpl->m_pFrameCounter->m_ProduceFrame;
        }

        // ==== 2) 一番古い未表示フレームが終わっていたら Present ====
        // presentFrame 〜 produceFrame-1 が「キック済み未表示」候補
        if (m_pImpl->m_pFrameCounter->m_TotalFrames < m_pImpl->m_pFrameCounter->m_ProduceFrame &&
            m_pImpl->updateJob.m_FinishedFrame >= m_pImpl->m_pFrameCounter->m_TotalFrames &&
            m_pImpl->renderJob.m_FinishedFrame >= m_pImpl->m_pFrameCounter->m_TotalFrames)
        {
            // const uint32_t presentIndex = static_cast<uint32_t>(m_pImpl->m_pFrameCounter->m_TotalFrames % Config::Graphics::BufferingCount);
#ifndef NDEBUG
            // ImGuiのフレーム開始
            m_pImpl->m_pImGuiManager->Begin();
            // エディタマネージャ更新
            m_pImpl->m_pEditorManager->Update();
            // ImGuiのフレーム終了
            m_pImpl->m_pImGuiManager->End();
#endif // !NDEBUG

            // ルーターがイベントを受けて、コマンドをQuereに積む
            m_pImpl->m_pRouterHub->FlushAll();
            // Present
            m_pImpl->m_pRenderer->Present();
            // FPS計測+Sleep制御
            m_pImpl->m_pFrameCounter->Tick();
        }
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

    /*======================== Platform ========================*/
    
    /*======================== Core ========================*/
    m_pImpl->m_pJobSystem->Initialize();

    /*======================== Graphics ========================*/
    if(!Core::LogAssert::Verify((Config::Graphics::BufferingCount == 2 || Config::Graphics::BufferingCount == 3),
        "Graphics Setting", "bufferingCount must be 2 or 3"))
    {
        return false;
    }
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
    if (!m_pImpl->m_pResourceManager->Initialize(m_pImpl->m_pRenderDevice.get(), m_pImpl->m_pDescriptorAllocator.get(), m_pImpl->m_pRenderer.get()))
    {
        Core::LogAssert::Verify(false, "ResourceManager Initialize", "ResourceManager initialization failed");
        return false;
    }
    // レンダラー初期化
    if (!m_pImpl->m_pRenderer->Initialize(m_pImpl->m_pRenderDevice.get(),m_pImpl->m_pResourceManager.get(),m_pImpl->m_pDescriptorAllocator.get()))
    {
        Core::LogAssert::Verify(false, "Renderer Initialize", "Renderer initialization failed");
        return false;
    }
    // シェーダーコンパイラー初期化
    if (!m_pImpl->m_pShaderCompiler->Initialize())
    {
        Core::LogAssert::Verify(false, "ShaderCompiler Initialize", "ShaderCompiler initialization failed");
        return false;
    }
    // パイプラインマネージャ初期化
    if(!m_pImpl->m_pPipelineManager->Initialize(m_pImpl->m_pRenderDevice->GetDevice(),m_pImpl->m_pShaderCompiler.get(),m_pImpl->m_pDescriptorAllocator.get()))
    {
        Core::LogAssert::Verify(false, "PipelineManager Initialize", "PipelineManager initialization failed");
        return false;
    }
    // スワップチェーン作成
    if (!m_pImpl->m_pRenderDevice->CreateSwapChain(m_pImpl->m_pDescriptorAllocator.get()))
    {
        Core::LogAssert::Verify(false, "SwapChain Create", "SwapChain creation failed");
        return false;
    }
    m_pImpl->m_pFrameCounter->SetMaxFPS(Config::Graphics::DisplayRefreshrate); // 0なら無制限
    m_pImpl->m_pFrameCounter->SetMaxLead(Config::Graphics::BufferingCount - 1); // 最大先行フレーム数
    m_pImpl->m_pFrameCounter->m_ProduceFrame = 0;
    m_pImpl->m_pFrameCounter->m_TotalFrames = 0;

#ifndef NDEBUG
    // ImGuiの初期化
    m_pImpl->m_pImGuiManager->Initialize(
        *m_pImpl->m_pRenderDevice.get(),
        *m_pImpl->m_pDescriptorAllocator.get());
    // Rendererにセット
    m_pImpl->m_pRenderer->SetImGuiManager(m_pImpl->m_pImGuiManager.get());
    // エディタマネージャ初期化
    m_pImpl->m_pEditorManager->Initialize(m_pImpl->m_pFrameCounter.get(), m_pImpl->m_pDescriptorAllocator.get(), m_pImpl->m_pFrameGraph.get());
#endif // !NDEBUG

    // デフォルトパス作成
    m_pImpl->m_pFrameGraph->CreateDefaultPasses();
    m_pImpl->m_pFrameGraph->Compile(*m_pImpl->m_pDescriptorAllocator.get(), *m_pImpl->m_pResourceManager.get());

    // デフォルトモデル生成
    m_pImpl->m_pModelContainer->CreateDefaultModels(*m_pImpl->m_pResourceManager.get());

    // 更新、描画用フレームジョブの作成
    m_pImpl->updateJob.Start(
        [&]([[maybe_unused]] uint64_t frameNo, uint32_t idx)
        {
            Update(idx);
        });
    m_pImpl->renderJob.Start(
        [&]([[maybe_unused]] uint64_t frameNo, uint32_t idx)
        {
            Render(idx);
        });

    // 初回バッファ埋め
    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        Update(i);
    }

    return true;
}

/// @brief エンジン終了処理
void Theatria::Engine::Shutdown()
{
    // ジョブシステムのスレッド停止
    m_pImpl->m_pJobSystem->StopAllThreads();
    // フレームジョブの停止
    m_pImpl->renderJob.Stop();
    m_pImpl->updateJob.Stop();
#ifndef NDEBUG
    // ImGuiの終了処理
    m_pImpl->m_pImGuiManager->Shutdown();
#endif // !NDEBUG
    // ウィンドウの破棄
    Platform::WinApp::TerminateWindow();
}

void Theatria::Engine::Update([[maybe_unused]] uint32_t frameIdx)
{

    Graphics::CopyCommandContext* copyCmd = m_pImpl->m_pRenderer->BeginCopyPass();

    // コピー
    Graphics::GpuBuffer& objectBuf = m_pImpl->m_pResourceManager->GetGlobalObjectBuffer<Graphics::ShaderStruct::SObject>().GetGpuBuffer(frameIdx);
    Graphics::GpuBuffer& transformBuf = m_pImpl->m_pResourceManager->GetGlobalTransformBuffer<Graphics::ShaderStruct::STransform>().GetGpuBuffer(frameIdx);
    Graphics::GpuBuffer& meshInfoBuf = m_pImpl->m_pResourceManager->GetGlobalMeshInfoBuffer<Graphics::ShaderStruct::SMeshInfo>().GetGpuBuffer(frameIdx);
    Graphics::GpuBuffer& upObjectBuf = m_pImpl->m_pResourceManager->GetGlobalUploadBuffer(Graphics::GlobalBufferType::ObjectBuffer);
    Graphics::GpuBuffer& upTransformBuf = m_pImpl->m_pResourceManager->GetGlobalUploadBuffer(Graphics::GlobalBufferType::TransformBuffer);
    Graphics::GpuBuffer& upMeshInfoBuf = m_pImpl->m_pResourceManager->GetGlobalUploadBuffer(Graphics::GlobalBufferType::MeshInfoBuffer);

    copyCmd->CopyResource(objectBuf.GetResource(), upObjectBuf.GetResource());
    copyCmd->CopyResource(transformBuf.GetResource(), upTransformBuf.GetResource());
    copyCmd->CopyResource(meshInfoBuf.GetResource(), upMeshInfoBuf.GetResource());

#ifndef NDEBUG
    Graphics::GpuBuffer& debugCamBuf = m_pImpl->m_pResourceManager->GetDebugCamBuf(frameIdx);
    Graphics::GpuBuffer& upDebugCamBuf = m_pImpl->m_pResourceManager->GetDebugCamUploadBuf();
    copyCmd->CopyResource(debugCamBuf.GetResource(), upDebugCamBuf.GetResource());
#endif // !NDEBUG

    m_pImpl->m_pRenderer->EndCopyPass(copyCmd);;
}

void Theatria::Engine::Render([[maybe_unused]] uint32_t frameIdx)
{
    m_pImpl->m_pFrameGraph->Execute(frameIdx, *m_pImpl->m_pRenderer.get(), *m_pImpl->m_pDescriptorAllocator.get(), *m_pImpl->m_pResourceManager.get(), *m_pImpl->m_pPipelineManager.get());
}
