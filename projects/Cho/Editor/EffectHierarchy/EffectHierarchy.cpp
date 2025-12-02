#include "pch.h"
#include "EffectHierarchy.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "GameCore/GameCore.h"

void EffectHierarchy::Initialize()
{
}

void EffectHierarchy::Update()
{
	Window();
}

void EffectHierarchy::Window()
{
	// 移動を無効にするフラグ
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove;
	ImGui::Begin("Effect Hierarchy", nullptr, windowFlags);

	if (!m_EditorManager->GetEffectEntity().has_value())
	{
		ImGui::Text("No Root");
		ImGui::End();
		return;
	}
	else
	{
		ImGui::Text("Effect Root");
	}
	EffectComponent* effect = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<EffectComponent>(m_EditorManager->GetEffectEntity().value());
	if (!effect)
	{
		ImGui::Text("No Effect");
		ImGui::End();
		return;
	}
	uint32_t nodeIndex = 0;
	for (auto& node : effect->root.second.nodes)
	{
		// ツリーノードとして表示
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
		if (m_EditorManager->GetEffectNodeID().has_value() && m_EditorManager->GetEffectNodeID().value() == node.nodeID)
		{
			flags |= ImGuiTreeNodeFlags_Selected; // 選択中のオブジェクトをハイライト
		}
		std::string label = node.name + "##" + std::to_string(node.nodeID);
		bool isTreeOpen = ImGui::TreeNodeEx(label.c_str(), flags);

		// 親ノードが左クリックされた場合の処理
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			// 選択中のオブジェクトを更新
			m_EditorManager->SetEffectNodeIndex(nodeIndex);
		}
		// 右クリックされた場合の処理
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			// 選択中のオブジェクトを更新
			m_EditorManager->SetEffectNodeIndex(nodeIndex);
			//ImGui::OpenPopup("HierarchyPopupMenu");
		}
		// ダブルクリックで編集モードに切り替え
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			//editing = true;
			//editingName = objectName;                       // 編集中の名前を設定
			//wcsncpy_s(inputBuffer, IM_ARRAYSIZE(inputBuffer), objectName.c_str(), _TRUNCATE); // 現在の名前をバッファにコピー
		}
		// ツリーノード展開処理
		if (isTreeOpen)
		{
			// 子ノードを表示する場合の処理
			ImGui::TreePop();
		}
		nodeIndex++;
	}

	ImGui::End();
}
