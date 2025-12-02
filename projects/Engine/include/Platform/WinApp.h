#pragma once
#include <Windows.h>
#include <wrl.h>
namespace Theatria
{
    namespace Graphics
    {
        class RenderDevice;
    }
    namespace Platform
    {
        class Graphics::RenderDevice;
        /// @brief WinAppクラス
        class WinApp final
        {
            friend class Graphics::RenderDevice;
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
            [[nodiscard]] static bool ProcessMessage();
        public:
            static UINT64       m_WindowWidth;  ///< ウィンドウ幅
            static UINT         m_WindowHeight; ///< ウィンドウ高さ
        private:
            static HWND         m_HWND; ///< ウィンドウハンドル
            static WNDCLASS     m_WC;   ///< ウィンドウクラス
        };
    }// namespace Platform
}// namespace Theatria

