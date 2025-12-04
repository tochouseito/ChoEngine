#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/AssetBrowser.h"
#include <imgui.h>

void Theatria::Editor::AssetBrowser::Initialize()
{
}

void Theatria::Editor::AssetBrowser::Update()
{
    ImGui::Begin("Asset Browser");
    ImGui::Text("This is the Asset Browser window.");
    ImGui::End();
}

#endif // !NDEBUG
