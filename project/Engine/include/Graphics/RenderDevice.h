#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
#include <queue>
#include <array>
#include <cstdint>
#include <mutex>
#include <condition_variable>

namespace Theatria::Graphics
{
    /// @brief レンダリングデバイス所有者。Buffer,Texture,Heap,PSO,RootSignature等のファクトリー
    class RenderDevice final
    {
        friend class DescriptorAllocator;

        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        class CommandContext
        {
        public:
            /// @brief コンストラクタ
            CommandContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
            /// @brief デストラクタ
            ~CommandContext() = default;

            void Reset();

            void Close();


            ID3D12CommandList* GetCommandList() noexcept { return m_List.Get(); }
            ID3D12CommandAllocator* GetCommandAllocator() noexcept { return m_Allocator.Get(); }
        private:
            ComPtr<ID3D12GraphicsCommandList> m_List = nullptr;
            ComPtr<ID3D12CommandAllocator> m_Allocator = nullptr;
        };

        class GraphicsCommandContext final : public CommandContext
        {
        public:
            /// @brief コンストラクタ
            GraphicsCommandContext(ID3D12Device* device)
                : CommandContext(device, D3D12_COMMAND_LIST_TYPE_DIRECT)
            {
            }
            /// @brief デストラクタ
            ~GraphicsCommandContext() = default;
        };

        class ComputeCommandContext final : public CommandContext
        {
        public:
            /// @brief コンストラクタ
            ComputeCommandContext(ID3D12Device* device)
                : CommandContext(device, D3D12_COMMAND_LIST_TYPE_COMPUTE)
            {
            }
            /// @brief デストラクタ
            ~ComputeCommandContext() = default;
        };

        class CopyCommandContext final : public CommandContext
        {
        public:
            /// @brief コンストラクタ
            CopyCommandContext(ID3D12Device* device)
                : CommandContext(device, D3D12_COMMAND_LIST_TYPE_COPY)
            {
            }
            /// @brief デストラクタ
            ~CopyCommandContext() = default;
        };

        class CommandPool final
        {
        public:
            /// @brief コンストラクタ
            CommandPool(ID3D12Device* device)
                : m_Device(device)
            {
            }
            /// @brief デストラクタ
            ~CommandPool() = default;

            GraphicsCommandContext* GetGraphicsContext()
            {
                std::lock_guard<std::mutex> lock(m_GraphicsMutex);
                if (m_GraphicsCtxPool.empty())
                {
                    auto context = std::make_unique<GraphicsCommandContext>(m_Device);
                    return context.release();
                }
                auto context = std::move(m_GraphicsCtxPool.front());
                m_GraphicsCtxPool.pop();
                return context.release();
            }

            ComputeCommandContext* GetComputeContext()
            {
                std::lock_guard<std::mutex> lock(m_ComputeMutex);
                if (m_ComputeCtxPool.empty())
                {
                    auto context = std::make_unique<ComputeCommandContext>(m_Device);
                    return context.release();
                }
                auto context = std::move(m_ComputeCtxPool.front());
                m_ComputeCtxPool.pop();
                return context.release();
            }

            CopyCommandContext* GetCopyContext()
            {
                std::lock_guard<std::mutex> lock(m_CopyMutex);
                if (m_CopyCtxPool.empty())
                {
                    auto context = std::make_unique<CopyCommandContext>(m_Device);
                    return context.release();
                }
                auto context = std::move(m_CopyCtxPool.front());
                m_CopyCtxPool.pop();
                return context.release();
            }

            void ReturnContext(GraphicsCommandContext* context)
            {
                std::lock_guard<std::mutex> lock(m_GraphicsMutex);
                m_GraphicsCtxPool.push(std::unique_ptr<GraphicsCommandContext>(context));
            }

            void ReturnContext(ComputeCommandContext* context)
            {
                std::lock_guard<std::mutex> lock(m_ComputeMutex);
                m_ComputeCtxPool.push(std::unique_ptr<ComputeCommandContext>(context));
            }

            void ReturnContext(CopyCommandContext* context)
            {
                std::lock_guard<std::mutex> lock(m_CopyMutex);
                m_CopyCtxPool.push(std::unique_ptr<CopyCommandContext>(context));
            }
        private:
            ID3D12Device* m_Device = nullptr;

            std::mutex m_GraphicsMutex;
            std::queue<std::unique_ptr<GraphicsCommandContext>> m_GraphicsCtxPool;
            std::mutex m_ComputeMutex;
            std::queue<std::unique_ptr<ComputeCommandContext>> m_ComputeCtxPool;
            std::mutex m_CopyMutex;
            std::queue<std::unique_ptr<CopyCommandContext>> m_CopyCtxPool;
        };

        enum class QueueType : uint8_t
        {
            Graphics,
            Compute,
            Copy,
            Count
        };

        class QueueContext 
        {
        public:
            /// @brief コンストラクタ
            QueueContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
            /// @brief デストラクタ
            ~QueueContext()
            {
                if (m_FenceEvent)
                {
                    CloseHandle(m_FenceEvent);
                    m_FenceEvent = nullptr;
                }
            }

            void Execute(CommandContext* ctx)
            {
                std::lock_guard<std::mutex> lock(m_FenceMutex);
                if (ctx)
                {
                    ID3D12CommandList* lists[] = { ctx->GetCommandList()};
                    m_CommandQueue->ExecuteCommandLists(1, lists);
                }
                m_FenceValue++;
                // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
                m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue);
            }
            void WaitForFence()
            {
                // Fenceの値が指定したSignal値にたどり着いているか確認する
                // GetCompletedValueの初期値はFence作成時に渡した初期値
                if (m_Fence->GetCompletedValue() < m_FenceValue)
                {
                    // 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
                    m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
                    // イベント待つ
                    WaitForSingleObject(m_FenceEvent, INFINITE);
                }
            }

            ID3D12CommandQueue* GetCommandQueue() noexcept { return m_CommandQueue.Get(); };
            ID3D12Fence* GetFence() noexcept { return m_Fence.Get(); };
            uint64_t GetFenceValue() noexcept { return m_FenceValue; };
        private:
            ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
            ComPtr<ID3D12Fence> m_Fence = nullptr;
            HANDLE m_FenceEvent = {};
            uint64_t m_FenceValue = {};
            std::mutex m_FenceMutex;
            std::condition_variable m_FenceCV;
        };

        class GraphicsQueueContext final : public QueueContext
        {
            public:
            /// @brief コンストラクタ
            GraphicsQueueContext(ID3D12Device* device)
                : QueueContext(device, D3D12_COMMAND_LIST_TYPE_DIRECT)
            {
            }
            /// @brief デストラクタ
            ~GraphicsQueueContext() = default;
        };

        class ComputeQueueContext final : public QueueContext
        {
            public:
            /// @brief コンストラクタ
            ComputeQueueContext(ID3D12Device* device)
                : QueueContext(device, D3D12_COMMAND_LIST_TYPE_COMPUTE)
            {
            }
            /// @brief デストラクタ
            ~ComputeQueueContext() = default;
        };

        class CopyQueueContext final : public QueueContext
        {
            public:
            /// @brief コンストラクタ
            CopyQueueContext(ID3D12Device* device)
                : QueueContext(device, D3D12_COMMAND_LIST_TYPE_COPY)
            {
            }
            /// @brief デストラクタ
            ~CopyQueueContext() = default;
        };

        class QueuePool final
        {
            public:
            /// @brief コンストラクタ
            QueuePool(ID3D12Device* device)
                : m_Device(device)
            {
                // グラフィックスキューのプールを初期化
                for (uint32_t i = 0; i < kGraphicsQueueCount; ++i)
                {
                    m_GraphicsQueuePool.push(std::make_unique<GraphicsQueueContext>(m_Device));
                }
                // コンピュートキューのプールを初期化
                for (uint32_t i = 0; i < kComputeQueueCount; ++i)
                {
                    m_ComputeQueuePool.push(std::make_unique<ComputeQueueContext>(m_Device));
                }
                // コピーキューのプールを初期化
                for (uint32_t i = 0; i < kCopyQueueCount; ++i)
                {
                    m_CopyQueuePool.push(std::make_unique<CopyQueueContext>(m_Device));
                }
            }
            /// @brief デストラクタ
            ~QueuePool() = default;
            GraphicsQueueContext* GetGraphicsQueue()
            {
                std::unique_lock<std::mutex> lock(m_GraphicsMutex);
                m_GraphicsCV.wait(lock, [this]() { return !m_GraphicsQueuePool.empty(); });
                auto queue = std::move(m_GraphicsQueuePool.front());
                m_GraphicsQueuePool.pop();
                return queue.release();
            }
            ComputeQueueContext* GetComputeQueue()
            {
                std::unique_lock<std::mutex> lock(m_ComputeMutex);
                m_ComputeCV.wait(lock, [this]() { return !m_ComputeQueuePool.empty(); });
                auto queue = std::move(m_ComputeQueuePool.front());
                m_ComputeQueuePool.pop();
                return queue.release();
            }
            CopyQueueContext* GetCopyQueue()
            {
                std::unique_lock<std::mutex> lock(m_CopyMutex);
                m_CopyCV.wait(lock, [this]() { return !m_CopyQueuePool.empty(); });
                auto queue = std::move(m_CopyQueuePool.front());
                m_CopyQueuePool.pop();
                return queue.release();
            }

            void ReturnQueue(GraphicsQueueContext* queue)
            {
                {
                    std::lock_guard<std::mutex> lock(m_GraphicsMutex);
                    m_GraphicsQueuePool.push(std::unique_ptr<GraphicsQueueContext>(queue));
                }
                m_GraphicsCV.notify_one();
            }

            void ReturnQueue(ComputeQueueContext* queue)
            {
                {
                    std::lock_guard<std::mutex> lock(m_ComputeMutex);
                    m_ComputeQueuePool.push(std::unique_ptr<ComputeQueueContext>(queue));
                }
                m_ComputeCV.notify_one();
            }

            void ReturnQueue(CopyQueueContext* queue)
            {
                {
                    std::lock_guard<std::mutex> lock(m_CopyMutex);
                    m_CopyQueuePool.push(std::unique_ptr<CopyQueueContext>(queue));
                }
                m_CopyCV.notify_one();
            }

        private:
            ID3D12Device* m_Device = nullptr;
            // 各キューの数
            static const uint32_t kGraphicsQueueCount = 2;///> 通常用途とPresent用
            static const uint32_t kComputeQueueCount = 4; ///>
            static const uint32_t kCopyQueueCount = 2;    ///>
            std::mutex m_GraphicsMutex;
            std::condition_variable m_GraphicsCV;
            std::queue<std::unique_ptr<GraphicsQueueContext>> m_GraphicsQueuePool;
            std::mutex m_ComputeMutex;
            std::condition_variable m_ComputeCV;
            std::queue<std::unique_ptr<ComputeQueueContext>> m_ComputeQueuePool;
            std::mutex m_CopyMutex;
            std::condition_variable m_CopyCV;
            std::queue<std::unique_ptr<CopyQueueContext>> m_CopyQueuePool;
        };

    public:
        RenderDevice() = default;
        ~RenderDevice() = default;

        /// @brief 初期化
        [[nodiscard]] bool Initialize(bool enableDebugLayer = false);

        /// @brief 直接Deviceを取得する
        ID3D12Device* operator->() { return m_Device.Get(); }
        const ID3D12Device* operator->() const { return m_Device.Get(); }
    private:
        /// @brief DXGIファクトリーの生成
        /// @param enableDebugLayer 
        [[nodiscard]] bool CreateDXGIFactory([[maybe_unused]] bool enableDebugLayer);
        /// @brief デバイスの生成
        [[nodiscard]] bool CreateDevice();
        /// @brief 各サポートチェック
        void CheckD3D12Options() noexcept ;
    private:
        ComPtr<ID3D12Device> m_Device = nullptr;///> D3D12デバイス
        ComPtr<IDXGIFactory7> m_DXGIFactory = nullptr;///> DXGIファクトリ

        std::unique_ptr<CommandPool> m_CommandPool = nullptr;///> コマンドコンテキストプール
        std::unique_ptr<QueuePool> m_QueuePool = nullptr;///> キュープール

        ComPtr<IDXGISwapChain4> m_SwapChain = nullptr;///> スワップチェイン
        DXGI_SWAP_CHAIN_DESC1 m_SwapChainDesc = {};///> スワップチェイン記述子
        int32_t m_RefreshRate = {};///> リフレッシュレート


        /*==================== D3D12Options ====================*/
        D3D12_FEATURE_DATA_D3D12_OPTIONS m_Options = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 m_Options1 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS2 m_Options2 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 m_Options3 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS4 m_Options4 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 m_Options5 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 m_Options6 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 m_Options7 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS8 m_Options8 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS9 m_Options9 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS10 m_Options10 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS11 m_Options11 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 m_Options12 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS13 m_Options13 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS14 m_Options14 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS15 m_Options15 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS16 m_Options16 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS17 m_Options17 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS18 m_Options18 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS19 m_Options19 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS20 m_Options20 = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS21 m_Options21 = {};
    };
};

