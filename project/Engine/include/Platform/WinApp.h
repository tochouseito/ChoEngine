#pragma once
#include <Windows.h>
#include <wrl.h>
namespace Theatria::Platform
{
    /// @brief WinAppクラス
    class WinApp final
    {
    public:
        /// @brief ウィンドウプロシージャ
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        /// @brief ウィンドウ作成
        /// @return 成功ならtrue、失敗ならfalse
        static bool CreateWindowApp();

        /// @brief ウィンドウ表示
        static void ShowWindowApp();

        /// @brief ウィンドウ破棄
        static void TerminateWindow();

        /// @brief ウィンドウメッセージ処理
        /// @return 終了ならtrue、継続ならfalse
        static bool ProcessMessage();
    private:
        static HWND         m_HWND; ///< ウィンドウハンドル
        static WNDCLASS     m_WC;   ///< ウィンドウクラス
        static UINT64       m_WindowWidth;  ///< ウィンドウ幅
        static UINT         m_WindowHeight; ///< ウィンドウ高さ
    };
};

