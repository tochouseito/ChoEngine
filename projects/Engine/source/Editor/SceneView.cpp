#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/SceneView.h"
#include <imgui.h>

void Theatria::Editor::SceneView::Initialize()
{
}

void Theatria::Editor::SceneView::Update()
{
    ImGui::Begin("Scene View");
    ImGui::Text("This is the Scene View window.");
    ImGui::End();
}

#endif // !NDEBUG
