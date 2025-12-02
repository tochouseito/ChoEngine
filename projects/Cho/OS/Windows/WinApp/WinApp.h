#pragma once

/*--------------------------------------------
WinAppクラス
--------------------------------------------*/

#define NOMINMAX // Windowのminmaxマクロを除外
#include <Windows.h>
#include <ole2.h>
#include <timeapi.h>
#include <cstdint>

class WinApp
{
public:// 静的メンバ関数

	// ウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:// メンバ関数

	// デストラクタ
	~WinApp() = default;

	// ゲームウィンドウの作成
	static void CreateGameWindow();

	// メッセージ処理
	static bool ProcessMessage();

	static bool IsEndApp();

	// ウィンドウのタイトルバー変更
	static void SetWindowTitle(const wchar_t* title);

	// ウィンドウの破棄
	static void TerminateWindow();

	// webを開く
	static void OpenWebURL(const wchar_t* url);

	/*Getter*/
	static HWND GetHWND() { return m_HWND; }
	static HINSTANCE GetHInstance() { return m_WC.hInstance; }
	static UINT64 GetWindowWidth() { return m_WindowWidth; }
	static UINT GetWindowHeight() { return m_WindowHeight; }
	static bool IsResizeWindow();
	static bool IsKillfocusWindow();
	static std::vector<std::wstring>& GetDropFiles() { return m_DropFiles; }
	static void ClearDropFiles() { m_DropFiles.clear(); }
	static bool IsDropFiles();
private:

	// ウィンドウサイズ変更時の処理
	static void OnWindowResize(UINT64 width, UINT height);

private:// メンバ変数
	static HWND m_HWND; // ウィンドウハンドル
	static WNDCLASS m_WC; // ウィンドウクラス
	static bool m_IsRun; // アプリケーションが動作中かを示すフラグ
	static UINT64 m_WindowWidth; // ウィンドウの幅
	static UINT m_WindowHeight; // ウィンドウの高さ
	static bool m_IsResize; // ウィンドウサイズ変更フラグ
	static bool m_IsKillfocus; // ウィンドウがフォーカスを失ったかどうか
	static bool m_IsDropFiles; // ドロップされたファイルがあるかどうか
	static std::vector<std::wstring> m_DropFiles; // ドロップされたファイルのパス
};

