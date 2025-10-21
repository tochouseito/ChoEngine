#include "pch.h"
#include "include/Platform/WinApp.h"
#include <cstdint>
#include <timeapi.h>
#include <shellapi.h>
#include <ole2.h>
#include "../ChoEditor/resource.h"
#pragma comment(lib,"winmm.lib")

// === ImGui ===
#include <External/imgui/include/imgui.h>
#include <External/imgui/include/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/// @brief ウィンドウプロシージャ
LRESULT Theatria::Platform::WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
    {
        return true;
    }
    // ウィンドウのメッセージ処理
    switch (msg)
    {
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lparam);
        pMinMaxInfo->ptMinTrackSize.x = 800; // 最小幅を設定（例：800）
        pMinMaxInfo->ptMinTrackSize.y = 600; // 最小高さを設定（例：600）
        break;
    }
    }
    // 標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/// @brief ウィンドウ作成
/// @return 成功ならtrue、失敗ならfalse
bool Theatria::Platform::WinApp::CreateWindowApp()
{
    // ウィンドウプロシージャ
    m_WC.lpfnWndProc = WindowProc;
    // ウィンドウクラス名
    m_WC.lpszClassName = L"TheatriaWindowClass";
    // インスタンスハンドル
    m_WC.hInstance = GetModuleHandle(nullptr);
    // カーソル
    m_WC.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // アイコン
    m_WC.hIcon = LoadIcon(m_WC.hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    // ウィンドウクラスを登録する
    RegisterClass(&m_WC);
    // ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = { 0,0,
        static_cast<LONG>(m_WindowWidth),
        static_cast<LONG>(m_WindowHeight) };
    // クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
    // ウィンドウの生成
    m_HWND = CreateWindow(
        m_WC.lpszClassName,     // 利用するクラス名
        L"Theatria",            // タイトルバーの文字
        WS_OVERLAPPEDWINDOW,    // よく見るウィンドウスタイル
        CW_USEDEFAULT,          // ウィンドウ横位置
        CW_USEDEFAULT,          // ウィンドウ縦位置
        wrc.right - wrc.left,   // ウィンドウ幅
        wrc.bottom - wrc.top,   // ウィンドウ高
        nullptr,                // 親ウィンドウハンドル
        nullptr,                // メニューハンドル
        m_WC.hInstance,         // インスタンスハンドル
        nullptr);               // オプション
    // システムタイマーの分解能を上げる
    timeBeginPeriod(1);

    return true;
}

/// @brief ウィンドウ表示
void Theatria::Platform::WinApp::ShowWindowApp()
{
    // ウィンドウを最大化して表示
    ShowWindow(m_HWND, SW_MAXIMIZE);
    // ドラッグ＆ドロップを有効化
    DragAcceptFiles(m_HWND, TRUE);
}

/// @brief ウィンドウ破棄
void Theatria::Platform::WinApp::TerminateWindow()
{
    // ウィンドウの破棄
    DestroyWindow(m_HWND);
    // ウィンドウクラスの登録解除
    UnregisterClass(m_WC.lpszClassName, m_WC.hInstance);
    // システムタイマーの分解能を元に戻す
    timeEndPeriod(1);
}

/// @brief ウィンドウメッセージ処理
/// @return 終了ならtrue、継続ならfalse
bool Theatria::Platform::WinApp::ProcessMessage()
{
    MSG msg{};
    // メッセージループ
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return true; // 終了メッセージが来たらtrueを返す
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return false;
}
