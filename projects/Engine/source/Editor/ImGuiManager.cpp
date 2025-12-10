#include "pch.h"
#ifndef NDEBUG
// === Theatria Engine Include ===
#include "config/engineConfig.h"
#include "include/Editor/ImGuiManager.h"
#include "include/Core/LogAssert.h"
#include "include/Platform/WinApp.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/Renderer.h"
#include "config/engineConfig.h"
// === C++ Standard Library ===
#include <filesystem>
// ===== ImGui =====
#include <External/imgui/include/imgui.h>
#include <External/imgui/include/imgui_impl_win32.h>
#include <External/imgui/include/imgui_impl_dx12.h>
// ===== ImGuizmo =====
#include <External/imgui_Extensions/ImGuizmo/ImGuizmo.h>
// ===== ImGui Node Editor =====
#include <External/imgui_Extensions/imgui-node-editor/imgui_node_editor.h>
using namespace Theatria::Editor;
using namespace Theatria::Platform;
using namespace Theatria::Graphics;

bool ImGuiManager::Initialize(RenderDevice& rdevice, Graphics::DescriptorAllocator& da)
{
    // imgui バージョン表示
    IMGUI_CHECKVERSION();
    std::string version = IMGUI_VERSION;
    Core::LogAssert::LogRuntime(std::source_location::current(), Core::LogAssert::SinkKind::Console,
        Core::LogAssert::LogLevel::Info, "ImGui", "ImGui Version: {}", version);
    // コンテキストの作成
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();/*
    io.DisplaySize = ImVec2(
        static_cast<float>(WinApp::m_WindowWidth),
        static_cast<float>(WinApp::m_WindowHeight));*/
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Dockingを有効化
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // マルチビューポートを有効化
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // キーボードナビゲーションを有効化
    // io.IniFilename = nullptr; // 設定ファイルを無効化
    // 設定ファイルの読み込み
    LoadIni();
    // プラットフォームのバックエンドを設定する
    ImGui_ImplWin32_Init(WinApp::m_HWND);
    // レンダラーのバックエンドを設定する
    DescriptorAllocator::TableID tableId = da.Allocate(DescriptorAllocator::TableKind::Buffers);
    ID3D12DescriptorHeap* cbvSrvHeap = da.GetDescriptorHeap(HeapType::CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE imguiCpu =
        cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE imguiGpu =
        cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
    
    ImGui_ImplDX12_Init(
        rdevice.GetDevice(),
        Config::Graphics::BufferingCount,
        Config::Graphics::DefaultDXGIFormat,
        cbvSrvHeap, // ★ GPU 可視ヒープ
        imguiCpu,           // ★ 同じヒープの CPU ハンドル
        imguiGpu            // ★ 同じヒープの GPU ハンドル
    );

    // フォントの設定
    ImFontConfig font_config;
    font_config.MergeMode = false;
    font_config.PixelSnapH = true;
    static const ImWchar japaneseRanges[] = {
    0x0020, 0x00FF,  // ASCII
    0x3000, 0x30FF,  // 句読点・ひらがな・カタカナ
    0x4E00, 0x9FFF,  // 漢字
    0xFF00, 0xFFEF,  // 全角英数
    0,
    };
    io.Fonts->AddFontFromFileTTF(
        "package/Fonts/NotoSansJP-Regular.ttf",// フォントファイルのパス
        16.0f,// フォントファイルのパスとフォントサイズ
        &font_config,
        japaneseRanges
    );
    // アイコンフォントをマージ
    font_config.MergeMode = true;
    static const ImWchar icon_ranges[] = { 0xf000, 0xf3ff, 0 }; // FontAwesomeの範囲
    io.Fonts->AddFontFromFileTTF(
        "package/Fonts/Font Awesome 6 Free-Solid-900.otf",
        14.0f,
        &font_config, icon_ranges);
    // Material Symbols Rounded をマージ
    static const ImWchar materialSymbolRanges[] = { 0xe000, 0xf8ff, 0 }; // Private Use Area
    io.Fonts->AddFontFromFileTTF(
        "package/Fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
        30.0f,
        &font_config,
        materialSymbolRanges
    );
    // 標準フォントを追加する
    io.Fonts->Build(); // フォントをビルド

    // ImGui スタイルの設定
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    // ビューポートが有効な場合、ウィンドウの角を丸くしない
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    // ツリーラインの表示
    style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;
    m_Initialized = true;
    return true;
}

void ImGuiManager::Shutdown()
{
    if (!m_Initialized) { return;}
    // 設定ファイルの保存
    SaveIni();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiManager::Begin()
{
    // ===== ImGui フレーム開始 =====
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    // ===== ImGuizmo フレーム開始 =====
    ImGuizmo::BeginFrame();
}

void ImGuiManager::End()
{
    // ===== ImGui フレーム終了 =====
    ImGui::Render();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

void ImGuiManager::Draw(CommandContext& ctx)
{
    ID3D12GraphicsCommandList* cmdList = ctx.GetCommandList();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void Theatria::Editor::ImGuiManager::SaveIni()
{
    namespace fs = std::filesystem;

    fs::path path = Config::FilePath::ImGui_iniPath;

    // 親ディレクトリを全部作成（存在してたら何もしない）
    fs::create_directories(path.parent_path());

    ImGui::SaveIniSettingsToDisk(path.string().c_str());
}

void Theatria::Editor::ImGuiManager::LoadIni()
{
    namespace fs = std::filesystem;
    fs::path path = Config::FilePath::ImGui_iniPath;
    ImGui::SaveIniSettingsToDisk(path.string().c_str());
}

#endif
