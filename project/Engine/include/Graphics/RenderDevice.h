#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
#include "include/Graphics/GPUCommand.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/GpuBuffer.h"

#include <array>

namespace Theatria::Graphics
{
    constexpr uint32_t k_SwapChainBufferCount = 2; ///> スワップチェインバッファ数

    struct SwapChainBuffer final
    {
        std::unique_ptr<GpuResource> pResource; ///> リソース
        uint32_t backBufferIndex = 0; ///> バックバッファインデックス
        DescriptorAllocator::TableID rtvTableID; ///> RTVテーブルID
    };

    struct SwapChainContext final
    {
        ComPtr<IDXGISwapChain1> m_SwapChain = nullptr;///> スワップチェイン
        DXGI_SWAP_CHAIN_DESC1 m_Desc = {};///> スワップチェイン記述子
        int32_t m_RefreshRate = {};///> リフレッシュレート
        std::array<SwapChainBuffer, k_SwapChainBufferCount> m_BackBuffers = {};///> バックバッファ
    };

    /// @brief レンダリングデバイス所有者。Buffer,Texture,Heap,PSO,RootSignature等のファクトリー
    class RenderDevice final
    {
        friend class DescriptorAllocator;
        friend class ResourceManager;
        friend class Renderer;

        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        RenderDevice() = default;
        ~RenderDevice()
        {
            if (m_QueuePool)
            {
                m_QueuePool->FlushAll();
            }

            m_SwapChainContext.m_SwapChain.Reset();
            m_QueuePool.reset();
            
            m_Device.Reset();
            m_DXGIFactory.Reset();
        }

        /// @brief 初期化
        [[nodiscard]] bool Initialize(bool enableDebugLayer = false);

        /// @brief 直接Deviceを取得する
        ID3D12Device* operator->() { return m_Device.Get(); }
        const ID3D12Device* operator->() const { return m_Device.Get(); }
        ID3D12Device* GetDevice() { return m_Device.Get(); }
        /// @brief スワップチェーンの作成
        [[nodiscard]] bool CreateSwapChain(DescriptorAllocator* descAllocator, uint32_t width = 0, uint32_t height = 0, uint32_t refreshRate = 0);
    private:
        /// @brief DXGIファクトリーの生成
        /// @param enableDebugLayer 
        [[nodiscard]] bool CreateDXGIFactory([[maybe_unused]] bool enableDebugLayer);
        /// @brief デバイスの生成
        [[nodiscard]] bool CreateDevice();
        /// @brief 各サポートチェック
        void CheckD3D12Options() noexcept;
    private:
        ComPtr<IDXGIFactory7> m_DXGIFactory = nullptr;///> DXGIファクトリ
        ComPtr<ID3D12Device> m_Device = nullptr;///> D3D12デバイス
        std::unique_ptr<QueuePool> m_QueuePool = nullptr;///> キュープール
        SwapChainContext m_SwapChainContext; ///> スワップチェーンコンテキスト


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

