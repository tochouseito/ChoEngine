#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/GameView.h"
#include <imgui.h>

void Theatria::Editor::GameView::Initialize()
{
}

void Theatria::Editor::GameView::Update()
{
    ImGui::Begin("Game View");
    ImGui::Text("This is the Game View window.");
    ImGui::End();
}

#endif // !NDEBUG
