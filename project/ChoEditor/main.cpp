#pragma warning(push)
#pragma warning(disable:28251)
// エンジン起動関数を使う
#define ENGINECREATE_FUNCTION
#define RUNTIMEMODE_EDITOR
// Engine
//#include "Engine/include/Main/Engine.h"
//#include "include/Utility/EngineCreateAPI.h"
#include "Cho/TheatriaEngineAPI.h"
// Windows
#include <Windows.h>
// C++
#include <memory>
#include <string>

# ifdef _DEBUG
#include <crtdbg.h>
# endif

class BuildWatcherController
{
public:
    BuildWatcherController() { ZeroMemory(&pi, sizeof(pi)); }
    ~BuildWatcherController() { TerminateBuildWatcher(); }

    // BuildWatcher起動
    void LaunchBuildWatcher()
    {
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        if (!CreateProcessW(
            L"BuildWatcher.exe",  // 実行ファイル
            nullptr,              // コマンドライン引数
            nullptr, nullptr,     // セキュリティ属性
            FALSE,                // ハンドル継承
            CREATE_NO_WINDOW,     // 完全非表示
            nullptr,              // 環境変数
            nullptr,              // カレントディレクトリ
            &si, &pi))
        {
            DWORD err = GetLastError();
            std::wstring msg = L"BuildWatcher.exe 起動失敗\nエラーコード: " + std::to_wstring(err);
            MessageBoxW(nullptr, msg.c_str(), L"Error", MB_ICONERROR);
        }
    }

    // BuildWatcher終了
    void TerminateBuildWatcher()
    {
        if (pi.hProcess)
        {
            TerminateProcess(pi.hProcess, 0); // ★強制終了（将来的にIPCで置換すると良い）
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            pi.hProcess = nullptr;
            pi.hThread = nullptr;
        }
    }

private:
    PROCESS_INFORMATION pi;
};

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#pragma warning(pop)
#ifdef _DEBUG
    //// メモリリーク検出
    //_CrtMemState s1, s2, diff;
    //// 開始時の状態を保存
    //_CrtMemCheckpoint(&s1);
    // 文字通り「デバッグ時のみ」有効化
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |= _CRTDBG_ALLOC_MEM_DF;     // デバッグヒープ有効
    flags |= _CRTDBG_LEAK_CHECK_DF;    // 終了時にリーク自動ダンプ
    _CrtSetDbgFlag(flags);
#endif

	// エンジンのインスタンス
	std::unique_ptr<Engine, decltype(&theatria::DestroyEngine)> engine(
		theatria::CreateEngine(RuntimeMode::Editor), theatria::DestroyEngine);// エンジンの生成
	theatria::SetEngine(engine.get());// エンジンのポインタをセット
    //std::unique_ptr<Theatria::Engine> engine =
    //    std::make_unique<Theatria::Engine>();// エンジンの生成
    //std::unique_ptr<Theatria::Engine, decltype(&Theatria::API::DestroyEngine)> engine(
    //    Theatria::API::CreateEngine(Theatria::RuntimeMode::RuntimeMode_Editor), Theatria::API::DestroyEngine);// エンジンの生成
    //Theatria::API::SetEngine(engine.get());// エンジンのポインタをセット

	// BuildWatcherコントローラー
	BuildWatcherController buildWatcher;

	// エンジンの稼働
	engine->CrashHandlerEntry();// クラッシュ時の処理のハンドラー
	buildWatcher.LaunchBuildWatcher();// BuildWatcher起動
	engine->Operation();// エンジンの稼働
    //Theatria::API::RunEngine();

    // 解放
    engine.reset();

#ifdef _DEBUG
    //// 終了時の状態を保存
    //_CrtMemCheckpoint(&s2);

    //// 差分を計算してリークを確認
    //if (_CrtMemDifference(&diff, &s1, &s2))
    //{
    //    OutputDebugStringA("[Memory Leak Detected in Scope!]\n");
    //    _CrtMemDumpStatistics(&diff);
    //}
    //else
    //{
    //    OutputDebugStringA("[No Leak in Scope]\n");
    //}
    //new int[10];
#endif

	return 0;
}
