#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
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
        enum class QueueType : uint8_t
        {
            Graphics,
            Compute,
            Copy,
            Count
        };

        class QueueContext final
        {
        public:
            /// @brief コンストラクタ
            QueueContext()
            {
                
            }
            /// @brief デストラクタ
            ~QueueContext()
            {
                if (m_FenceEvent)
                {
                    CloseHandle(m_FenceEvent);
                    m_FenceEvent = nullptr;
                }
            }

            bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
            {
                // フェンスの作成
                m_Fence.Reset();
                m_FenceValue = 0;// 初期値0でFenceを作る
                if (!Core::LogAssert::Verify(
                    device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)),
                    "RenderDevice",
                    "Failed to create fence."))
                {
                    return false;
                }
                // FenceのSignalを持つためのイベントを作成する
                m_FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                if(!Core::LogAssert::Verify(
                    m_FenceEvent != nullptr,
                    "RenderDevice",
                    "Failed to create fence event."))
                {
                    return false;
                }
                // コマンドキューの作成
                D3D12_COMMAND_QUEUE_DESC desc = {};
                desc.Type = type;
                if(!Core::LogAssert::Verify(
                    SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue))),
                    "RenderDevice",
                    "Failed to create command queue."))
                {
                    return false;
                }
                return true;
            }
            void Execute(ID3D12GraphicsCommandList* commandList)
            {
                std::lock_guard<std::mutex> lock(m_FenceMutex);
                if (commandList)
                {
                    ID3D12CommandList* lists[] = { commandList };
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
        [[nodiscard]] bool CreateDXGIFactory(bool enableDebugLayer);
        /// @brief デバイスの生成
        [[nodiscard]] bool CreateDevice();
        /// @brief 各サポートチェック
        void CheckD3D12Options() noexcept ;
    private:
        ComPtr<ID3D12Device> m_Device = nullptr;///> D3D12デバイス
        ComPtr<IDXGIFactory7> m_DXGIFactory = nullptr;///> DXGIファクトリ

        // 各キューの数
        static const uint32_t kGraphicsQueueCount = 2;///> 通常用途とPresent用
        static const uint32_t kComputeQueueCount = 4; ///>
        static const uint32_t kCopyQueueCount = 2;    ///>

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

