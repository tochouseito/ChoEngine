#include "pch.h"
#include "ImGuiManager.h"
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <ImGuizmo/ImGuizmo.h>
#include <imgui-node-editor/imgui_node_editor.h>
#include "OS/Windows/WinApp/WinApp.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace ed = ax::NodeEditor;

void ImGuiManager::Initialize(ID3D12Device8* device, ResourceManager* resourceManager)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	std::string version = IMGUI_VERSION;
	Log::Write(LogLevel::Info, version);
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Dockingを有効化
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // マルチビューポートを有効化
	// プラットフォームとレンダラーのバックエンドを設定する
	ImGui_ImplWin32_Init(WinApp::GetHWND());

	/*ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = device;*/
	m_DescriptorHeapIndex = resourceManager->GetSUVDHeap()->Allocate();
	ImGui_ImplDX12_Init(
		device, 2,
		PixelFormat, resourceManager->GetSUVDHeap()->GetDescriptorHeap(),
		resourceManager->GetSUVDHeap()->GetCPUDescriptorHandle(m_DescriptorHeapIndex.value()),
		resourceManager->GetSUVDHeap()->GetGPUDescriptorHandle(m_DescriptorHeapIndex.value())
	);

	// 日本語フォント
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
	io.Fonts->AddFontFromFileTTF(//"C:/Windows/Fonts/msgothic.ttc",
		"Cho/Resources/EngineAssets/Fonts/NotoSansJP-Regular.ttf",// フォントファイルのパス
		16.0f,// フォントファイルのパスとフォントサイズ
		&font_config,
		japaneseRanges
	);
	// アイコンフォントをマージ
	font_config.MergeMode = true;
	static const ImWchar icon_ranges[] = { 0xf000, 0xf3ff, 0 }; // FontAwesomeの範囲
	io.Fonts->AddFontFromFileTTF(
		"Cho/Resources/EngineAssets/Fonts/Font Awesome 6 Free-Solid-900.otf",
		14.0f,
		&font_config, icon_ranges);
	// Material Symbols Rounded をマージ
	static const ImWchar materialSymbolRanges[] = { 0xe000, 0xf8ff, 0 }; // Private Use Area
	io.Fonts->AddFontFromFileTTF(
		"Cho/Resources/EngineAssets/Fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
		30.0f,
		&font_config,
		materialSymbolRanges
	);
	// 標準フォントを追加する
	// io.Fonts->AddFontDefault();
	io.Fonts->Build(); // フォントをビルド
	
	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;
}

void ImGuiManager::Finalize()
{
	// 後始末
	//ImGui::DestroyPlatformWindows();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiManager::Begin()
{
	// ImGuiフレーム開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();     // ← ImGuizmo のフレーム開始
}

void ImGuiManager::End()
{
	// 描画前準備
	ImGui::Render();
	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
}

void ImGuiManager::Draw(ID3D12GraphicsCommandList6* commandList)
{
	// 描画コマンドを発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
