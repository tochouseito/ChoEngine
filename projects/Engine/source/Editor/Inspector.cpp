#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/Inspector.h"
#include <imgui.h>

void Theatria::Editor::Inspector::Initialize()
{
}

void Theatria::Editor::Inspector::Update()
{
    ImGui::Begin("Inspector");
    ImGui::Text("This is the Inspector window.");
    ImGui::End();
}
#endif // !NDEBUG

