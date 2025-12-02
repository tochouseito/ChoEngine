#pragma once

/*--------------------------------------------
DirectX12共通クラス
--------------------------------------------*/

#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"

class DirectX12Common
{
public:// メンバ関数

	// Constructor
	DirectX12Common();
	// Destructor
	~DirectX12Common();

	// 初期化
	void Initialize();

	// 終了処理
	void Finalize();

	/*Getter*/
	ID3D12Device8* GetDevice() const { return m_Device.Get(); }
	IDXGIFactory7* GetDXGIFactory() const { return m_DXGIFactory.Get(); }

private:// メンバ関数

	// DXGIファクトリーの生成
	void CreateDXGIFactory(const bool& enableDebugLayer);

	// デバイスの生成
	void CreateDevice();

    // 各サポートチェック
    void CheckD3D12Features();

private:// メンバ変数
	ComPtr<IDXGIFactory7> m_DXGIFactory = nullptr;// DXGIファクトリ
	ComPtr<ID3D12Device8> m_Device = nullptr;// デバイス

    // d3d11on12
    ComPtr<ID3D11Device> m_D3D11Device = nullptr;
    ComPtr<ID3D11DeviceContext> m_D3D11Context = nullptr;
    ComPtr<ID3D11On12Device> m_D3D11On12Device = nullptr;

    // d2d / DWrite
    ComPtr<ID2D1Factory3> m_D2DFactory = nullptr;
    ComPtr<ID2D1Device> m_D2DDevice = nullptr;
    ComPtr<ID2D1DeviceContext5> m_D2DContext = nullptr;
    ComPtr<IDWriteFactory5> m_DWriteFactory = nullptr;

	// D3D12Options
	D3D12_FEATURE_DATA_D3D12_OPTIONS m_Options;
	D3D12_FEATURE_DATA_D3D12_OPTIONS1 m_Options1;
	D3D12_FEATURE_DATA_D3D12_OPTIONS2 m_Options2;
	D3D12_FEATURE_DATA_D3D12_OPTIONS3 m_Options3;
	D3D12_FEATURE_DATA_D3D12_OPTIONS4 m_Options4;
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 m_Options5;
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 m_Options6;
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 m_Options7;
	D3D12_FEATURE_DATA_D3D12_OPTIONS8 m_Options8;
	D3D12_FEATURE_DATA_D3D12_OPTIONS9 m_Options9;
	D3D12_FEATURE_DATA_D3D12_OPTIONS10 m_Options10;
	D3D12_FEATURE_DATA_D3D12_OPTIONS11 m_Options11;
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 m_Options12;
	D3D12_FEATURE_DATA_D3D12_OPTIONS13 m_Options13;
	D3D12_FEATURE_DATA_D3D12_OPTIONS14 m_Options14;
	D3D12_FEATURE_DATA_D3D12_OPTIONS15 m_Options15;
	D3D12_FEATURE_DATA_D3D12_OPTIONS16 m_Options16;
	D3D12_FEATURE_DATA_D3D12_OPTIONS17 m_Options17;
	D3D12_FEATURE_DATA_D3D12_OPTIONS18 m_Options18;
	D3D12_FEATURE_DATA_D3D12_OPTIONS19 m_Options19;
	D3D12_FEATURE_DATA_D3D12_OPTIONS20 m_Options20;
	D3D12_FEATURE_DATA_D3D12_OPTIONS21 m_Options21;
};

/*--------------------------------------------
リソースリークチェッカー
--------------------------------------------*/
class ResourceLeakChecker
{
public:// メンバ関数

	// デストラクタ
	~ResourceLeakChecker();
};

