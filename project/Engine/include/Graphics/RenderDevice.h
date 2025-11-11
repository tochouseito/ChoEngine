#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>

namespace Theatria::Graphics
{
    /// @brief レンダリングデバイス所有者。Buffer,Texture,Heap,PSO,RootSignature等のファクトリー
    class RenderDevice final
    {
        friend class DescriptorAllocator;

        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
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

        ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            uint32_t numDescriptors,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE) const;
    private:
        ComPtr<ID3D12Device> m_Device = nullptr;///> D3D12デバイス
        ComPtr<IDXGIFactory7> m_DXGIFactory = nullptr;///> DXGIファクトリ

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

