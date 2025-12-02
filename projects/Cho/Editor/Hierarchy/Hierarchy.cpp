#include "pch.h"
#include "Hierarchy.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "EngineCommand/EngineCommands.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "GameCore/GameCore.h"
#include "Core/ChoLog/ChoLog.h"
#include <wchar.h>
#include "GameCore/GameWorld/GameWorld.h"

void Hierarchy::Initialize()
{
}

void Hierarchy::Update()
{
	Window();
}

void Hierarchy::Window()
{
	// 移動を無効にするフラグ
	//ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove;
	ImGui::Begin("Hierarchy", nullptr);
	// ドロップターゲット
	//if (ImGui::BeginDragDropTarget())
	//{
	//	if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ModelData"))
	//	{
	//		// ドロップされたModelDataの名前を取得
	//		const char* ModelName = static_cast<const char*>(payload->Data);
	//		ModelData* modelData = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelData(ConvertString(ModelName));
	//		// メッシュの数分生成
	//		for (auto& meshData : modelData->meshes)
	//		{
	//			// モデルデータからオブジェクトを生成
	//			std::unique_ptr<Add3DObjectCommand> add3DObject = std::make_unique<Add3DObjectCommand>();
	//			add3DObject->Execute(m_EngineCommand);
	//			// 生成したオブジェクト
	//			GameObject* object = m_EngineCommand->GetGameCore()->GetObjectContainer()->GetGameObject(add3DObject->GetObjectID());
	//			TransformComponent* transform = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<TransformComponent>(object->GetEntity());
	//			// 必要なコンポーネントを付与
	//			// メッシュ
	//			std::unique_ptr<AddMeshFilterComponent> addMeshComp = std::make_unique<AddMeshFilterComponent>(object->GetEntity());
	//			addMeshComp->Execute(m_EngineCommand);
	//			MeshFilterComponent* meshFilter = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MeshFilterComponent>(object->GetEntity());
	//			meshFilter->modelName = modelData->name;
	//			meshFilter->modelID = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelDataIndex(meshFilter->modelName);
	//			m_EngineCommand->GetResourceManager()->GetModelManager()->RegisterModelUseList(meshFilter->modelID.value(), transform->mapID.value());
	//			std::unique_ptr<AddMeshRendererComponent> addRenderComp = std::make_unique<AddMeshRendererComponent>(object->GetEntity());
	//			addRenderComp->Execute(m_EngineCommand);
	//			//MeshRendererComponent* meshRenderer = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MeshRendererComponent>(object->GetEntity());
	//			// マテリアル
	//			if (!meshData.materials.empty())
	//			{
	//				std::unique_ptr<AddMaterialComponent> addMaterialComp = std::make_unique<AddMaterialComponent>(object->GetEntity());
	//				addMaterialComp->Execute(m_EngineCommand);
	//				MaterialComponent* material = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MaterialComponent>(object->GetEntity());
	//				if (!meshData.materials[0].textureName.empty())
	//				{
	//					material->enableTexture = true;
	//					material->textureName = ConvertString(meshData.materials[0].textureName);
	//					material->textureID = m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureID(material->textureName);
	//				}
	//			}
	//			// アニメーション
	//			if (!modelData->animations.empty())
	//			{
	//				std::unique_ptr<AddAnimationComponent> addAnimationComp = std::make_unique<AddAnimationComponent>(object->GetEntity());
	//				addAnimationComp->Execute(m_EngineCommand);
	//				//AnimationComponent* animation = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<AnimationComponent>(object->GetEntity());
	//			}
	//		}
	//	}
	//	ImGui::EndDragDropTarget();
	//}
	
	// ワールドのオブジェクトを表示
	GameWorld* gameWorld = m_EngineCommand->GetGameCore()->GetGameWorld();

	static bool editing = false;
	static wchar_t inputBuffer[128] = L""; // 入力用バッファ
	static std::wstring editingName;   // 現在編集中のオブジェクト名
	// オブジェクトをリストで表示
	if(gameWorld->GetWorldContainer().empty() || gameWorld->GetWorldContainer()[0].empty())
	{
		ImGui::Text("No GameObject");
		ImGui::End();
		return;
	}
	for (const auto& objects : gameWorld->GetWorldContainer()[0])
	{
		for (const auto& object : objects)
		{
			if (editing && editingName == object->GetName())
			{
				// 編集モード: InputTextを表示
				std::string inputName = ConvertString(inputBuffer);
				char sInputBuffer[128] = "";
				strncpy_s(sInputBuffer, IM_ARRAYSIZE(sInputBuffer), inputName.c_str(), _TRUNCATE);
				if (ImGui::InputText("##edit", sInputBuffer, IM_ARRAYSIZE(sInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					std::string newName(sInputBuffer);
					std::wstring wNewName = ConvertString(newName);

					// 名前が空でない場合に更新
					if (!wNewName.empty())
					{
						// オブジェクトの名前を更新
						std::unique_ptr<RenameObjectCommand> renameCommand = std::make_unique<RenameObjectCommand>(object->GetHandle(), wNewName);
						m_EngineCommand->ExecuteCommand(std::move(renameCommand));

						editing = false;  // 編集終了
					}
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemActive())
				{
					editing = false;
				}
			}
			else
			{
				// オブジェクトの名前を取得
				std::wstring objectName = object->GetName();
				// ツリーノードとして表示
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
				// 選択中のオブジェクトをハイライトするためのフラグ
				if (m_EditorManager->GetSelectedGameObject() && m_EditorManager->GetSelectedGameObject()->GetHandle().entity == object->GetHandle().entity)
				{
					flags |= ImGuiTreeNodeFlags_Selected; // 選択中のオブジェクトをハイライト
				}
				bool isTreeOpen = ImGui::TreeNodeEx(ConvertString(objectName).c_str(), flags);

				// 親ノードが左クリックされた場合の処理
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					// 選択中のオブジェクトを更新
					m_EditorManager->SetSelectedGameObject(object.get());
				}
				// 右クリックされた場合の処理
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					// 選択中のオブジェクトを更新
					m_EditorManager->SetSelectedGameObject(object.get());
					ImGui::OpenPopup("HierarchyPopupMenu");
				}
				// ダブルクリックで編集モードに切り替え
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					editing = true;
					editingName = objectName;                       // 編集中の名前を設定
					wcsncpy_s(inputBuffer, IM_ARRAYSIZE(inputBuffer), objectName.c_str(), _TRUNCATE); // 現在の名前をバッファにコピー
				}
				// ツリーノード展開処理
				if (isTreeOpen)
				{
					// 子ノードを表示する場合の処理
					ImGui::TreePop();
				}
			}
		}
	}
	// ポップアップメニュー
	if (ImGui::BeginPopup("HierarchyPopupMenu"))
	{
		if (ImGui::MenuItem("削除"))
		{
			// 選択中のオブジェクトを削除
			std::unique_ptr<DeleteObjectCommand> deleteCommand = std::make_unique<DeleteObjectCommand>(m_EditorManager->GetSelectedGameObject()->GetHandle());
			m_EngineCommand->ExecuteCommand(std::move(deleteCommand));
			// 選択中オブジェクトをクリア
			m_EditorManager->SetSelectedGameObject(nullptr);
			// ポップアップメニューを閉じる
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("複製"))
		{
			// 選択中のオブジェクトを複製
			std::unique_ptr<CopyObjectCommand> copyCommand = std::make_unique<CopyObjectCommand>(m_EditorManager->GetSelectedGameObject()->GetHandle());
			m_EngineCommand->ExecuteCommand(std::move(copyCommand));
			// ポップアップメニューを閉じる
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::End();
}
