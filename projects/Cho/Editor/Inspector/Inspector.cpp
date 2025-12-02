#include "pch.h"
#include "Inspector.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommands.h"
#include "GameCore/GameCore.h"
#include "Core/ChoLog/ChoLog.h"
#include "TheaReflectDeclare.h"
using namespace theatria;
#include "Resources/ResourceManager/ResourceManager.h"

void Inspector::Initialize()
{
}

void Inspector::Update()
{
	Window();
}

void Inspector::Window()
{
	// 移動を無効にするフラグ
	//ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove;
	ImGui::Begin("Inspector", nullptr);

	// 選択中のオブジェクトがある場合に情報を表示
	if (m_EditorManager->GetSelectedGameObject())
	{
		// 選択中のオブジェクトを取得
		GameObject* object = m_EditorManager->GetSelectedGameObject();
		// オブジェクトの名前を取得
		std::wstring objectName = object->GetName();
		// オブジェクトのタイプを取得
		ObjectType objectType = object->GetType();
		// オブジェクト名とタイプを表示
		ImGui::Text("Name: %s", ConvertString(objectName).c_str());
		// タイプを表示
		ImGui::Text("Type: %s", ObjectTypeToWString(objectType));
		// Entity IDを表示
		ImGui::Text("Entity ID: %d", object->GetHandle().entity);
		// コンポーネントの情報を表示
		ComponentsView(object);
		// コンポーネントの追加ボタンを表示
		AddComponent(object);
	}

	ImGui::End();
}

void Inspector::ComponentsView(GameObject* object)
{
	TransformComponentView(object);
	MeshFilterComponentView(object);
	MeshRendererComponentView(object);
	CameraComponentView(object);
	UISpriteComponentView(object);
	MaterialComponentView(object);
	ScriptComponentView(object);
	LineRendererComponentView(object);
	Rigidbody2DComponentView(object);
	BoxCollider2DComponentView(object);
	Rigidbody3DComponentView(object);
	LightComponentView(object);
	EmitterComponentView(object);
	ParticleComponentView(object);
	AudioComponentView(object);
	AnimationComponentView(object);
}

void Inspector::TransformComponentView(GameObject* object)
{
	// TransformComponentを取得
	TransformComponent* transform = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<TransformComponent>(object->GetHandle().entity);
	if (!transform) { return; }
	// Transformを表示
	ImGui::SeparatorText("Transform"); // ラインとテキスト表示
	// 平行移動の操作
	ImGuiEx::ColoredDragFloat3("Translation", &transform->position.x, 0.01f, 0.0f, 0.0f, "%.1f");
	// 回転の操作
	ImGuiEx::ColoredDragFloat3("Rotation", &transform->degrees.x, 0.1f, 0.0f, 0.0f, "%.1f°");
	// スケールの操作
	ImGuiEx::ColoredDragFloat3("Scale", &transform->scale.x, 0.01f, 0.0f, 0.0f, "%.1f");
	// ビルボードフラグのチェックボックス
	ImGui::Checkbox("Billboard", &transform->isBillboard);
}

void Inspector::MeshFilterComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<MeshFilterComponent>(object->GetType())) { return; }
	// MeshFilterComponentを取得
	MeshFilterComponent* mesh = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MeshFilterComponent>(object->GetHandle().entity);
	if (!mesh) { return; }
	ImGui::Text("Mesh Component");
	ImGui::Text("Mesh Name: %s", ConvertString(mesh->modelName).c_str());
	if (ImGui::BeginMenu("MeshSelect"))
	{
		for (const auto& modelName : m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelNameContainer())
		{
			std::string name = ConvertString(modelName.first);
			if (ImGui::MenuItem(name.c_str()))
			{
				// Transformとの連携
				TransformComponent* transform = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<TransformComponent>(object->GetHandle().entity);
				// モデル名を設定
				mesh->modelName = modelName.first;
				// モデルのIDを取得
				mesh->modelID = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelDataIndex(mesh->modelName);
				// モデルのUseListに登録
				m_EngineCommand->GetResourceManager()->GetModelManager()->RegisterModelUseList(mesh->modelID.value(), transform->mapID.value());
			}
		}

		ImGui::EndMenu();
	}
}

void Inspector::MeshRendererComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<MeshRendererComponent>(object->GetType())) { return; }
	// MeshRendererComponentを取得
	MeshRendererComponent* render = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MeshRendererComponent>(object->GetHandle().entity);
	if (render) { return; }
	ImGui::Text("Render Component");
}

void Inspector::CameraComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<CameraComponent>(object->GetType())) { return; }
	// CameraComponentを取得
	CameraComponent* camera = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<CameraComponent>(object->GetHandle().entity);
	if (!camera) { return; }
	ImGui::Text("Camera Component");
}

void Inspector::UISpriteComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<UISpriteComponent>(object->GetType())) { return; }
	// UISpriteComponentを取得
	UISpriteComponent* sprite = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<UISpriteComponent>(object->GetHandle().entity);
	if (!sprite) { return; }
	ImGui::Text("Sprite Component");
	// position
	ImGui::DragFloat2("Position", &sprite->position.x, 1.0f, 0.0f, 0.0f, "%.1f");
	// rotation
	ImGui::DragFloat("Rotation", &sprite->rotation, 0.1f, 0.0f, 0.0f, "%.1f°");
	// scale
	ImGui::DragFloat2("Scale", &sprite->scale.x, 0.01f, 0.0f, 0.0f, "%.1f");
	// anchorpoint
	ImGui::DragFloat2("AnchorPoint", &sprite->anchorPoint.x, 0.01f, 0.0f, 0.0f, "%.1f");
	// size
	ImGui::DragFloat2("Size", &sprite->size.x, 0.01f, 0.0f, 0.0f, "%.1f");
	// texturelefttop
	ImGui::DragFloat2("TextureLeftTop", &sprite->textureLeftTop.x, 0.01f, 0.0f, 0.0f, "%.1f");
	// texturesize
	ImGui::DragFloat2("TextureSize", &sprite->textureSize.x, 0.01f, 0.0f, 0.0f, "%.1f");
}

void Inspector::MaterialComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<MaterialComponent>(object->GetType())) { return; }
	// MaterialComponentを取得
	MaterialComponent* material = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MaterialComponent>(object->GetHandle().entity);
	if (!material) { return; }
	ImGui::Text("Material Component");
	ImGui::ColorEdit4("Color", &material->color.r);
	ImGui::DragFloat("Shininess", &material->shininess, 0.1f, 0.0f, 0.0f, "%.1f");
	ImGui::DragFloat("cubeUVScale", &material->cubeUVScale, 0.01f, 0.0f, 0.0f, "%.2f");
	ImGui::Checkbox("ライティング", &material->enableLighting);
	ImGui::Checkbox("UseTexture", &material->enableTexture);
	if (material->enableTexture)
	{
		// フリップフラグ
		ImGui::Checkbox("uvFlipY", &material->uvFlipY);
		// テクスチャ名を表示
		if (material->textureName.empty())
		{
			ImGui::Text("Texture Name: None");
		} else
		{
			ImGui::Text("Texture Name: %s", ConvertString(material->textureName).c_str());
		}
		// ドロップターゲット
		if (ImGui::BeginDragDropTarget())
		{
			// テクスチャならそのまま使う
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Texture"))
			{
				// ドロップされたテクスチャのIDを取得
				const char* textureName = static_cast<const char*>(payload->Data);
				// テクスチャ名を設定
				material->textureName = ConvertString(textureName);
				// テクスチャIDを取得
				material->textureID = m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureID(material->textureName);
			}

			if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TextureID"))
			{
			}
			ImGui::EndDragDropTarget();
		}
		// CubeTexture
		if(material->textureName.empty())
		{
			ImGui::Text("Cube Texture Name: None");
		} else
		{
			ImGui::Text("Cube Texture Name: %s", ConvertString(material->cubeTextureName).c_str());
		}
		// ドロップターゲット
		if (ImGui::BeginDragDropTarget())
		{
			// キューブテクスチャならそのまま使う
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TextureID"))
			{
				// ドロップされたキューブテクスチャのIDを取得
				const char* cubeTextureName = static_cast<const char*>(payload->Data);
				// キューブテクスチャ名を設定
				material->cubeTextureName = ConvertString(cubeTextureName);
				// キューブテクスチャIDを取得
				TextureData* texData = m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureData(material->cubeTextureName);
				if (texData->metadata.IsCubemap())
				{
					material->cubeTextureID = m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureID(material->cubeTextureName);
				}
				else
				{
					material->cubeTextureID.reset();
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
}

void Inspector::ScriptComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<ScriptComponent>(object->GetType())) { return; }
	ScriptComponent* script = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<ScriptComponent>(object->GetHandle().entity);
	if (!script) return;
	ScriptContainer* scriptContainer = m_EngineCommand->GetResourceManager()->GetScriptContainer();
	if (!scriptContainer) return;

	ImGui::Text("Script Component");

    ScriptData* scriptData = scriptContainer->GetScriptDataByName(script->scriptName);

	// プルダウン用スクリプト名一覧作成
	std::vector<std::string> scriptNames = { "None" };
	for (size_t i = 0; i<scriptContainer->GetScriptCount();i++)
	{
        std::optional<ScriptData> data = scriptContainer->GetScriptDataByID(static_cast<uint32_t>(i));
		if (data)
		{
			scriptNames.push_back(data->scriptName);
		}
	}

	// 現在の選択位置を求める
	int currentIndex = 0;// 0番目は"None"
	for (int i = 1; i < scriptNames.size(); ++i)// 1番目から開始
	{
		if (script->scriptName == scriptNames[i])
		{
			currentIndex = i;
			break;
		}
	}

	// プルダウン表示
	if (ImGui::BeginCombo("Script", scriptNames.empty() ? "None" : scriptNames[currentIndex].c_str()))
	{
		for (int i = 0; i < scriptNames.size(); ++i)
		{
			bool isSelected = (i == currentIndex);
			if (ImGui::Selectable(scriptNames[i].c_str(), isSelected))
			{
				currentIndex = i;
				if (!i)
				{
					// "None"が選択されたらスクリプト無効化
					script->scriptName.clear();
					script->isActive = false;
				} else
				{
					// 選択されたスクリプト名とIDを設定
					script->scriptName = scriptNames[i];
					script->isActive = false;
				}
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

    if (script->instance)
    {
        for (const auto& field : script->fields)
        {
            if (field.second.type == typeid(int))
            {
                ImGui::DragInt(field.first.c_str(), static_cast<int*>(field.second.ptr), 1.0f, field.second.minmax.first, field.second.minmax.second);
            }
            else if (field.second.type == typeid(float))
            {
                ImGui::DragFloat(field.first.c_str(), static_cast<float*>(field.second.ptr), 0.1f, static_cast<float>(field.second.minmax.first), static_cast<float>(field.second.minmax.second));
            }
            else if (field.second.type == typeid(std::string))
            {
                /*char buffer[256];
                std::string* strPtr = static_cast<std::string*>(field.second.ptr);
                strncpy(buffer, strPtr->c_str(), sizeof(buffer));
                if (ImGui::InputText(field.first.c_str(), buffer, sizeof(buffer)))
                {
                    *strPtr = std::string(buffer);
                }*/
                ImGui::Text("%s", field.first.c_str());
            }
            else if (field.second.type == typeid(bool))
            {
                ImGui::Checkbox(field.first.c_str(), static_cast<bool*>(field.second.ptr));
            }
            else if (field.second.type == typeid(float3))
            {
                float3* vec = static_cast<float3*>(field.second.ptr);
                ImGui::DragFloat3(field.first.c_str(), &vec->x, 0.1f);
            }
            else
            {
                ImGui::Text("Unknown Type");
            }
        }
    }
    else
    {
        if (scriptData)
        {
            for (auto& field : scriptData->saveFields)
            {
                if (field.second.type == typeid(int))
                {
                    ImGui::DragInt(field.first.c_str(), &std::get<int>(field.second.value), 1.0f, field.second.minmax.first, field.second.minmax.second);
                }
                else if (field.second.type == typeid(float))
                {
                    ImGui::DragFloat(field.first.c_str(), &std::get<float>(field.second.value), 0.1f, static_cast<float>(field.second.minmax.first), static_cast<float>(field.second.minmax.second));
                }
                else if (field.second.type == typeid(std::string))
                {
                    /*char buffer[256];
                    std::string* strPtr = static_cast<std::string*>(field.second.ptr);
                    strncpy(buffer, strPtr->c_str(), sizeof(buffer));
                    if (ImGui::InputText(field.first.c_str(), buffer, sizeof(buffer)))
                    {
                        *strPtr = std::string(buffer);
                    }*/
                    ImGui::Text("%s", field.first.c_str());
                }
                else if (field.second.type == typeid(bool))
                {
                    ImGui::Checkbox(field.first.c_str(), &std::get<bool>(field.second.value));
                }
                else if (field.second.type == typeid(float3))
                {
                    float3 vec = std::get<float3>(field.second.value);
                    ImGui::DragFloat3(field.first.c_str(), &vec.x, 0.1f);
                }
                else
                {
                    ImGui::Text("Unknown Type");
                }
            }
        }
    }
}

void Inspector::LineRendererComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<LineRendererComponent>(object->GetType())) { return; }
	auto lines = m_EngineCommand->GetGameCore()->GetECSManager()->GetAllComponents<LineRendererComponent>(object->GetHandle().entity);

	if (!lines || lines->empty()) return;

	ImGui::Text("Line Renderer Components");

	// スクロール可能な子ウィンドウ
	ImGui::BeginChild("LineRendererList", ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	int index = 0;
	for (LineRendererComponent& line : *lines)
	{
		ImGui::PushID(index); // 各項目に一意IDをつける

		// 折りたたみヘッダーで表示をまとめる
		if (ImGui::CollapsingHeader(("Line " + std::to_string(index)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent();

			ImGui::InputFloat3("Start", &line.line.start.x);
			ImGui::InputFloat3("End", &line.line.end.x);
			ImGui::ColorEdit4("Color", &line.line.color.r);

			if (line.mapID.has_value())
			{
				ImGui::Text("MapID: %d", line.mapID.value());
			} else
			{
				ImGui::Text("MapID: None");
			}

			ImGui::Unindent();
		}

		ImGui::PopID();
		++index;
	}

	ImGui::EndChild();
}

void Inspector::Rigidbody2DComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<Rigidbody2DComponent>(object->GetType())) { return; }
	Rigidbody2DComponent* rigidbody = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<Rigidbody2DComponent>(object->GetHandle().entity);
	if (!rigidbody) { return; }
	if (ImGui::TreeNode("Rigidbody2D"))
	{
		// Active
		ImGui::Checkbox("Active", &rigidbody->isActive);

		// Body Type Combo
		const char* bodyTypeItems[] = { "Static", "Kinematic", "Dynamic" };
		int currentType = static_cast<int>(rigidbody->bodyType);
		if (ImGui::Combo("Body Type", &currentType, bodyTypeItems, IM_ARRAYSIZE(bodyTypeItems)))
		{
			rigidbody->bodyType = static_cast<physics::d2::Id2BodyType>(currentType);
		}

		// mass
		ImGui::DragFloat("mass", &rigidbody->mass, 0.01f);

		// Gravity Scale
		ImGui::DragFloat("Gravity Scale", &rigidbody->gravityScale, 0.1f, 0.0f, 100.0f);

		// Fixed Rotation
		ImGui::Checkbox("Fixed Rotation", &rigidbody->fixedRotation);

		// Is Kinematic（参考用）
		ImGui::Checkbox("Is Kinematic", &rigidbody->isKinematic);

		// コンポーネント削除
		if (ImGui::Button("Remove Rigidbody2D Component"))
		{
			m_EngineCommand->GetGameCore()->GetECSManager()->RemoveComponent<Rigidbody2DComponent>(object->GetHandle().entity);
		}

		ImGui::TreePop();
	}
	// 実行中の変更を反映させるためのボタン
	//if (ImGui::Button("Apply Rigidbody Changes"))
	//{
	//	if (rigidbody->runtimeBody)
	//	{
	//		b2World* world = GetPhysicsWorld(); // あなたのエンジンで取得
	//		world->DestroyBody(rigidbody->runtimeBody);
	//		rigidbody->runtimeBody = nullptr;
	//	}
	//}
}

void Inspector::BoxCollider2DComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<BoxCollider2DComponent>(object->GetType())) { return; }
	BoxCollider2DComponent* collider =
		m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<BoxCollider2DComponent>(object->GetHandle().entity);

	if (!collider) { return; }
	if (ImGui::TreeNode("BoxCollider2D"))
	{
		ImGui::DragFloat2("Offset", reinterpret_cast<float*>(&collider->offsetX), 0.1f);
		ImGui::DragFloat2("Size", reinterpret_cast<float*>(&collider->width), 0.1f, 0.01f, 1000.0f);

		ImGui::DragFloat("Density", &collider->density, 0.05f, 0.0f, 100.0f);
		ImGui::DragFloat("Friction", &collider->friction, 0.05f, 0.0f, 1.0f);
		ImGui::DragFloat("Restitution", &collider->restitution, 0.05f, 0.0f, 1.0f);

		ImGui::Checkbox("Is Sensor", &collider->isSensor);

		// コンポーネント削除
		if (ImGui::Button("Remove BoxCollider2D Component"))
		{
			m_EngineCommand->GetGameCore()->GetECSManager()->RemoveComponent<BoxCollider2DComponent>(object->GetHandle().entity);
		}

		ImGui::TreePop();
	}
}

void Inspector::Rigidbody3DComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<Rigidbody3DComponent>(object->GetType())) { return; }
	Rigidbody3DComponent* rigidbody = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<Rigidbody3DComponent>(object->GetHandle().entity);
	if (!rigidbody) { return; }
	if (ImGui::TreeNode("Rigidbody3D"))
	{
		// Active
		bool active = rigidbody->IsActive();
		ImGui::Checkbox("Active", &active);
		rigidbody->SetActive(active);
		// BodyParameters
		// 質量
		ImGui::DragFloat("質量", &rigidbody->mass, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Gravity Scale", &rigidbody->gravityScale, 0.1f, 0.0f, 100.0f);
		// 軸固定
		ImGui::Checkbox("X軸座標固定", &rigidbody->fixedPositionX);
		ImGui::Checkbox("Y軸座標固定", &rigidbody->fixedPositionY);
		ImGui::Checkbox("Z軸座標固定", &rigidbody->fixedPositionZ);
		ImGui::Checkbox("X軸回転固定", &rigidbody->fixedRotationX);
		ImGui::Checkbox("Y軸回転固定", &rigidbody->fixedRotationY);
		ImGui::Checkbox("Z軸回転固定", &rigidbody->fixedRotationZ);
		// Body Type Combo
		const char* bodyTypeItems[] = { "Static", "Dynamic", "Kinematic" };
		int currentType = static_cast<int>(rigidbody->bodyType);
		if (ImGui::Combo("Body Type", &currentType, bodyTypeItems, IM_ARRAYSIZE(bodyTypeItems)))
		{
			rigidbody->bodyType = static_cast<physics::d3::Id3BodyType>(currentType);
		}
		// Sensor
		ImGui::Checkbox("Is Sensor", &rigidbody->isSensor);
		ImGui::DragFloat3("Half Size", &rigidbody->halfsize.x, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat3("Velocity", &rigidbody->velocity.x, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat3("Angular Velocity", &rigidbody->angularVelocity.x, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat("摩擦係数", &rigidbody->friction, 0.05f, 0.0f, 1.0f);
		ImGui::DragFloat("反発係数", &rigidbody->restitution, 0.05f, 0.0f, 1.0f);
		if (ImGui::Button("Remove Rigidbody3D Component"))
		{
			m_EngineCommand->GetGameCore()->GetECSManager()->RemoveComponent<Rigidbody3DComponent>(object->GetHandle().entity);
		}
		ImGui::TreePop();
	}
}

void Inspector::EmitterComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<EmitterComponent>(object->GetType())) { return; }
	EmitterComponent* emitter = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<EmitterComponent>(object->GetHandle().entity);
	if (!emitter) { return; }
	if (ImGui::Button("発生"))
	{
		emitter->emit = true;
	}
	ImGui::SeparatorText("生存時間");
	ImGui::DragFloat("中央値", &emitter->lifeTime.median, 0.1f, 0.0f, 120.0f);
	ImGui::DragFloat("振幅", &emitter->lifeTime.amplitude, 0.1f, 0.0f, 120.0f);
	ImGui::Separator();
	ImGui::DragInt("発生数", &emitter->emitCount, 1, 0, 1024);
	ImGui::Checkbox("フェードアウト", &emitter->isFadeOut);
	ImGui::Checkbox("ビルボード", &emitter->isBillboard);
	if (ImGui::BeginTabBar("Emitter Settings"))
	{
		// position
		if(ImGui::BeginTabItem("Position"))
		{
			EffectEditor::DragRandfloat3("位置", &emitter->position.value, 0.1f, 0.0f, 0.0f);
			EffectEditor::DragRandfloat3("速度", &emitter->position.velocity, 0.1f, 0.0f, 0.0f);
			EffectEditor::DragRandfloat3("加速度", &emitter->position.acceleration, 0.1f, 0.0f, 0.0f);
			ImGui::EndTabItem();
		}
		// rotation
		if (ImGui::BeginTabItem("Rotation"))
		{
			EffectEditor::DragRandfloat3("回転", &emitter->rotation.value, 0.1f, 0.0f, 0.0f);
			EffectEditor::DragRandfloat3("速度", &emitter->rotation.velocity, 0.1f, 0.0f, 0.0f);
			EffectEditor::DragRandfloat3("加速度", &emitter->rotation.acceleration, 0.1f, 0.0f, 0.0f);
			ImGui::EndTabItem();
		}
		// scale
		if (ImGui::BeginTabItem("Scale"))
		{
			EffectEditor::DragRandfloat3("スケール", &emitter->scale.value, 0.1f, 0.0f, 0.0f);
			EffectEditor::DragRandfloat3("速度", &emitter->scale.velocity, 0.1f, 0.0f, 0.0f);
			EffectEditor::DragRandfloat3("加速度", &emitter->scale.acceleration, 0.1f, 0.0f, 0.0f);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	//ImGui::DragFloat("Emit Frequency", &emitter->frequency, 0.1f, 0.0f, 100.0f);
	//ImGui::DragFloat("Emit frequencyTime", &emitter->frequencyTime, 0.1f, 0.0f, 100.0f);
}

void Inspector::ParticleComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<ParticleComponent>(object->GetType())) { return; }
	ParticleComponent* particle = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<ParticleComponent>(object->GetHandle().entity);
	if (!particle) { return; }
	ImGui::Text("Particle Component");
	/*if (ImGui::TreeNode("Particle"))
	{
		ImGui::DragFloat3("Position", &particle->position.x, 0.1f);
		ImGui::DragFloat("radius", &particle->radius, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("lifeTime", &particle->lifeTime, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("speed", &particle->speed, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("angle", &particle->angle, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("gravity", &particle->gravity, 0.1f, -100.0f, 100.0f);
		ImGui::TreePop();
	}*/
}

void Inspector::LightComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<LightComponent>(object->GetType())) { return; }
	LightComponent* light = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<LightComponent>(object->GetHandle().entity);
	if (!light) { return; }
	ImGui::Text("Light Component");
	ImGui::ColorEdit4("Color", &light->color.r);
	ImGui::DragFloat("Intensity", &light->intensity, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Range", &light->range, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Decay", &light->decay, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Spot Angle", &light->spotAngle, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Spot Falloff Start", &light->spotFalloffStart, 0.1f, 0.0f, 100.0f);
}

void Inspector::AudioComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<AudioComponent>(object->GetType())) { return; }
	AudioComponent* audio = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<AudioComponent>(object->GetHandle().entity);
	if (!audio) { return; }
	ImGui::Text("Audio Component");
	//ImGui::Text("Audio Name: %s", ConvertString(audio->audioName).c_str());
	//std::unordered_map<std::string, uint32_t> audioNameMap = m_EngineCommand->GetResourceManager()->GetAudioManager()->GetSoundDataToName();
	//// オーディオを選択するためのドロップダウンメニュー
	//if (ImGui::BeginCombo("Audio", audio->audioName.empty() ? "None" : audio->audioName.c_str()))
	//{
	//	for (const auto& pair : audioNameMap)
	//	{
	//		if (ImGui::Selectable(pair.first.c_str(), audio->audioName == pair.first))
	//		{
	//			audio->audioName = pair.first;
	//			audio->audioID = pair.second;
	//		}
	//	}
	//	ImGui::EndCombo();
	//}
	//// オーディオソースの追加
	//// オーディオを選択するためのドロップダウンメニュー
	//if (ImGui::BeginCombo("AddSource","TEST"))
	//{
	//	for (const auto& pair : audioNameMap)
	//	{
	//		for(const auto& src : audio->soundData)
	//		{
	//			// 既に追加されているものは選択不可にする
	//			if(src.name == pair.first) continue;
	//		}
	//		if (ImGui::Selectable(pair.first.c_str()))
	//		{
	//			// 追加
	//			SoundData soundData = m_EngineCommand->GetResourceManager()->GetAudioManager()->CreateSoundData(pair.first);
	//			audio->soundData.push_back(soundData);
	//		}
	//	}
	//	ImGui::EndCombo();
	//}
	//// 各オーディオソースの設定
	//for(auto& src : audio->soundData)
	//{
	//	ImGui::TreeNode(src.name.c_str());

	//	ImGui::DragFloat("Volume", &src.currentVolume, 0.01f, 0.0f, 1.0f);

	//	ImGui::TreePop();
	//}
}

void Inspector::AnimationComponentView(GameObject* object)
{
	// 許可されているコンポーネントか確認
	if (!IsComponentAllowedAtRuntime<AnimationComponent>(object->GetType())) { return; }
	AnimationComponent* animation = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<AnimationComponent>(object->GetHandle().entity);
	if (!animation) { return; }
	ImGui::Text("Animation Component");
	ModelData* modelData = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelData(animation->modelName);
	std::vector<AnimationData>& animations = modelData->animations;
	if (animations.empty()) { return; }
	// アニメーション名のリストを作成
	std::vector<std::string> animationNames;
	for (const auto& anim : animations)
	{
		animationNames.push_back(anim.name);
	}
	// 現在のアニメーション名を取得
	AnimationData& nowAnimation = modelData->animations[animation->animationIndex];
	if (ImGui::BeginCombo("Animation", nowAnimation.name.c_str()))
	{
		for (size_t i = 0; i < animationNames.size(); ++i)
		{
			bool isSelected = (i == animation->animationIndex);
			if (ImGui::Selectable(animationNames[i].c_str(), isSelected))
			{
				// アニメーションインデックスを更新
				animation->animationIndex = static_cast<uint32_t>(i);
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	// 遷移時間の設定
	ImGui::DragFloat("Transition Time", &animation->transitionDuration, 0.01f, 0.0f, 10.0f, "%.2f s");
}


void Inspector::AddComponent(GameObject* object)
{
	static bool isOpen = false;
	MeshFilterComponent* mesh;
	MeshRendererComponent* render;
	MaterialComponent* material;
	ScriptComponent* script;
	std::vector<LineRendererComponent>* lines;
	lines;
	Rigidbody2DComponent* rigidbody;
	BoxCollider2DComponent* boxCollider2d;
	Rigidbody3DComponent* rigidbody3d;
	EmitterComponent* emitter;
	ParticleComponent* particle;
	if (!isOpen)
	{
		// コンポーネントの追加
		if (ImGui::Button("Add Component"))
		{
			isOpen = true;
		}
	} else
	{
		ObjectType objectType = object->GetType();

		bool canAddMeshFilter = IsComponentAllowedAtRuntime<MeshFilterComponent>(objectType);
		bool canAddMeshRenderer = IsComponentAllowedAtRuntime<MeshRendererComponent>(objectType);
		bool canAddMaterial = IsComponentAllowedAtRuntime<MaterialComponent>(objectType);
		bool canAddScript = IsComponentAllowedAtRuntime<ScriptComponent>(objectType);
		bool canAddLineRenderer = IsComponentAllowedAtRuntime<LineRendererComponent>(objectType);
		bool canAddRigidbody2D = IsComponentAllowedAtRuntime<Rigidbody2DComponent>(objectType);
		bool canAddBoxCollider2D = IsComponentAllowedAtRuntime<BoxCollider2DComponent>(objectType);
		bool canAddRigidbody3D = IsComponentAllowedAtRuntime<Rigidbody3DComponent>(objectType);
		bool canAddEmitter = IsComponentAllowedAtRuntime<EmitterComponent>(objectType);
		bool canAddParticle = IsComponentAllowedAtRuntime<ParticleComponent>(objectType);
		bool canAddAnimation = IsComponentAllowedAtRuntime<AnimationComponent>(objectType);
		bool canAddAudio = IsComponentAllowedAtRuntime<AudioComponent>(objectType);

		if (canAddMeshFilter)
		{
			mesh = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MeshFilterComponent>(object->GetHandle().entity);
			if (!mesh)
			{
				if (ImGui::Selectable("MeshFilterComponent"))
				{
					// MeshFilterComponentを追加
					std::unique_ptr<AddMeshFilterComponent> addMeshComp = std::make_unique<AddMeshFilterComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addMeshComp));
					isOpen = false;
				}
			}
		}
		if (canAddMeshRenderer)
		{
			render = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MeshRendererComponent>(object->GetHandle().entity);
			if (!render)
			{
				if (ImGui::Selectable("MeshRendererComponent"))
				{
					// MeshRendererComponentを追加
					std::unique_ptr<AddMeshRendererComponent> addRenderComp = std::make_unique<AddMeshRendererComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addRenderComp));
					isOpen = false;
				}
			}
		}
		if (canAddMaterial)
		{
			material = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<MaterialComponent>(object->GetHandle().entity);
			if (!material)
			{
				if (ImGui::Selectable("MaterialComponent"))
				{
					// MaterialComponentを追加
					std::unique_ptr<AddMaterialComponent> addMaterialComp = std::make_unique<AddMaterialComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addMaterialComp));
					isOpen = false;
				}
			}
		}
		if (canAddScript)
		{
			script = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<ScriptComponent>(object->GetHandle().entity);
			if (!script)
			{
				if (ImGui::Selectable("ScriptComponent"))
				{
					// ScriptComponentを追加
					std::unique_ptr<AddScriptComponent> addScriptComp = std::make_unique<AddScriptComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addScriptComp));
					isOpen = false;
				}
			}
		}
		if (canAddLineRenderer)
		{
			if (ImGui::Selectable("LineRendererComponent"))
			{
				// LineRendererComponentを追加
				std::unique_ptr<AddLineRendererComponent> addLineComp = std::make_unique<AddLineRendererComponent>(object->GetHandle().entity);
				m_EngineCommand->ExecuteCommand(std::move(addLineComp));
				isOpen = false;
			}
		}
		if (canAddRigidbody2D)
		{
			rigidbody = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<Rigidbody2DComponent>(object->GetHandle().entity);
			if (!rigidbody)
			{
				if (ImGui::Selectable("Rigidbody2DComponent"))
				{
					// Rigidbody2DComponentを追加
					std::unique_ptr<AddRigidbody2DComponent> addRigidBodyComp = std::make_unique<AddRigidbody2DComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addRigidBodyComp));
					isOpen = false;
				}
			}
		}
		if (canAddBoxCollider2D)
		{
			boxCollider2d = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<BoxCollider2DComponent>(object->GetHandle().entity);
			if (!boxCollider2d)
			{
				if (ImGui::Selectable("BoxCollider2DComponent"))
				{
					// BoxCollider2DComponentを追加
					std::unique_ptr<AddBoxCollider2DComponent> addBoxColliderComp = std::make_unique<AddBoxCollider2DComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addBoxColliderComp));
					isOpen = false;
				}
			}
		}
		if (canAddRigidbody3D)
		{
			rigidbody3d = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<Rigidbody3DComponent>(object->GetHandle().entity);
			if (!rigidbody3d)
			{
				if (ImGui::Selectable("Rigidbody3DComponent"))
				{
					// Rigidbody3DComponentを追加
					std::unique_ptr<AddRigidbody3DComponent> addRigidBodyComp = std::make_unique<AddRigidbody3DComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addRigidBodyComp));
					isOpen = false;
				}
			}
		}
		if (canAddEmitter)
		{
			emitter = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<EmitterComponent>(object->GetHandle().entity);
			if (!emitter)
			{
				if (ImGui::Selectable("EmitterComponent"))
				{
					// EmitterComponentを追加
					std::unique_ptr<AddEmitterComponent> addEmitterComp = std::make_unique<AddEmitterComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addEmitterComp));
					isOpen = false;
				}
			}
		}
		if (canAddParticle)
		{
			particle = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<ParticleComponent>(object->GetHandle().entity);
			if (!particle)
			{
				if (ImGui::Selectable("ParticleComponent"))
				{
					// ParticleComponentを追加
					std::unique_ptr<AddParticleComponent> addParticleComp = std::make_unique<AddParticleComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addParticleComp));
					isOpen = false;
				}
			}
		}
		if(canAddAnimation)
		{
			// AnimationComponentを取得
			AnimationComponent* animation = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<AnimationComponent>(object->GetHandle().entity);
			if (!animation)
			{
				if (ImGui::Selectable("AnimationComponent"))
				{
					// AnimationComponentを追加
					std::unique_ptr<AddAnimationComponent> addAnimationComp = std::make_unique<AddAnimationComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addAnimationComp));
					isOpen = false;
				}
			}
		}
		if (canAddAudio)
		{
			AudioComponent* audio = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<AudioComponent>(object->GetHandle().entity);
			if (!audio)
			{
				if (ImGui::Selectable("AudioComponent"))
				{
					// AudioComponentを追加
					std::unique_ptr<AddAudioComponent> addAudioComp = std::make_unique<AddAudioComponent>(object->GetHandle().entity);
					m_EngineCommand->ExecuteCommand(std::move(addAudioComp));
					isOpen = false;
				}
			}
		}
	}
}
