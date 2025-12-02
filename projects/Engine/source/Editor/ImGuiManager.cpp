#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/ImGuiManager.h"
#include "include/Core/LogAssert.h"
#include "include/Platform/WinApp.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/GraphicsSetting.h"
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

bool ImGuiManager::Initialize(RenderDevice& rdevice, DescriptorAllocator& da)
{
    // imgui バージョン表示
    IMGUI_CHECKVERSION();
    std::string version = IMGUI_VERSION;
    Core::LogAssert::Log(std::source_location::current(), Core::LogAssert::SinkKind::Console,
        Core::LogAssert::LogLevel::Info, "ImGui", "ImGui Version: {}", version);
    // コンテキストの作成
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Dockingを有効化
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // マルチビューポートを有効化
    // プラットフォームのバックエンドを設定する
    ImGui_ImplWin32_Init(WinApp::m_HWND);
    // レンダラーのバックエンドを設定する
    DescriptorAllocator::TableID tableID = da.Allocate(DescriptorAllocator::TableKind::Textures);
    ImGui_ImplDX12_Init(
        rdevice.GetDevice()
        , Setting::BufferingCount,
        Setting::DefaultDXGIFormat,
        da.GetDescriptorHeap(HeapType::CBV_SRV_UAV),
        da.GetCPUHandle(tableID),
        da.GetGPUHandle(tableID));

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

    return true;
}

void ImGuiManager::Shutdown()
{
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
    ImGui::EndFrame();
    ImGui::Render();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

void ImGuiManager::Draw(CommandContext& ctx)
{
    ID3D12GraphicsCommandList* cmdList = ctx.GetCommandList();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

#endif
