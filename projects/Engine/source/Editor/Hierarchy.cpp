#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/Hierarchy.h"
#include <imgui.h>

void Theatria::Editor::Hierarchy::Initialize()
{
}

void Theatria::Editor::Hierarchy::Update()
{
    ImGui::Begin("Hierarchy");
    ImGui::Text("This is the Hierarchy window.");
    ImGui::End();
}

#endif // !NDEBUG

