#include "pch.h"
#include "EffectEditor.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "GameCore/GameCore.h"
#include "Core/Utility/FontCode.h"

void EffectEditor::Initialize()
{
}

void EffectEditor::Update()
{
	Window();
	ControlWindow();
}

void EffectEditor::Window()
{
	ImGui::Begin("Effect Editor");

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

	// nodeが選択されているか
	uint32_t nodeIndex = 0;
	if (m_EditorManager->GetEffectNodeID().has_value())
	{
		nodeIndex = m_EditorManager->GetEffectNodeID().value();
	}
	else
	{
		ImGui::Text("No Node");
		ImGui::End();
		return;
	}
	// nodeの情報を取得
	EffectNodeData& node = effect->root.second.nodes[nodeIndex];

	ImGui::Separator();
	if (ImGui::BeginTabBar("Node"))
	{
		// Common
		if (ImGui::BeginTabItem("Common"))
		{
			// 名前表示
			char nameBuffer[128];
			strncpy_s(nameBuffer, sizeof(nameBuffer), node.name.c_str(), _TRUNCATE);
			if (ImGui::InputText("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer)))
			{
				node.name = std::string(nameBuffer);
			}
			// 最大生成数
			int maxCount = static_cast<int>(node.common.emitCountMax);
			if (ImGui::DragInt("最大生成数", &maxCount, 1, 0, 1024))
			{
				node.common.emitCountMax = static_cast<uint32_t>(maxCount);
			}
			// 同時生成数
			int emitCount = static_cast<int>(node.common.emitCount);
			if (ImGui::DragInt("同時生成数", &emitCount, 1, 0, 1024))
			{
				node.common.emitCount = static_cast<uint32_t>(emitCount);
			}
			// 無限生成フラグ
			bool isUnlimited = node.common.isUnlimited != 0 ? true : false;
			ImGui::Checkbox("無限生成", &isUnlimited);
			node.common.isUnlimited = isUnlimited ? 1 : 0;
			// 位置影響タイプ
			// 回転影響タイプ
			// スケール影響タイプ
			// 寿命で削除フラグ
			// 親削除で削除フラグ
			// 全ての子削除で削除フラグ
			// 生存時間
			ImGui::DragFloat2("生存時間", &node.common.lifeTime.median, 1.0f, 0.0f, 0.0f, "%.0f");
			// 発生間隔
			ImGui::DragFloat2("発生間隔", &node.common.emitInterval.median, 0.1f, 0.0f, 0.0f);
			// 生成開始時間
			ImGui::DragFloat2("生成開始時間", &node.common.emitStartTime.median, 0.1f, 0.0f, 0.0f);

			// End
			ImGui::EndTabItem();
		}
		// Position
		if (ImGui::BeginTabItem("位置"))
		{
			// PositionType
			static const char* positionTypeStrings[] = {
				"Standard",     // PositionStandard
				"PVA",          // PositionPVA
				//"Easing",       // PositionEasing
				//"FCurve",       // PositionFCurve
				//"NURBS",        // PositionNURBS
				//"CameraOffset", // PositionCameraOffset
			};
			int positionType = static_cast<int>(node.position.type);
			if (ImGui::Combo("Position Type", &positionType, positionTypeStrings, IM_ARRAYSIZE(positionTypeStrings)))
			{
				node.position.type = static_cast<uint32_t>(positionType);
			}
			// Position
			EFFECT_SRT_TYPE type = static_cast<EFFECT_SRT_TYPE>(node.position.type);
			switch (type)
			{
			case EFFECT_SRT_TYPE::SRT_TYPE_STANDARD:
				ImGui::DragFloat3("位置", &node.position.value.x, 0.1f);
				break;
				break;
			case EFFECT_SRT_TYPE::SRT_TYPE_PVA:
				DragRandfloat3("位置", &node.position.pva.value, 0.1f, 0.0f, 0.0f);
				DragRandfloat3("速度", &node.position.pva.velocity, 0.1f, 0.0f, 0.0f);
				DragRandfloat3("加速度", &node.position.pva.acceleration, 0.1f, 0.0f, 0.0f);
				break;
			case EFFECT_SRT_TYPE::SRT_TYPE_EASING:
				break;
			default:
				break;
			}

			// End
			ImGui::EndTabItem();
		}
		// Rotation
		if (ImGui::BeginTabItem("回転"))
		{
			// RotationType
			static const char* rotationTypeStrings[] = {
				"Standard",     // RotationStandard
				"PVA",          // RotationPVA
				//"Easing",       // RotationEasing
				//"FCurve",       // RotationFCurve
				//"NURBS",        // RotationNURBS
				//"CameraOffset", // RotationCameraOffset
			};
			int rotationType = static_cast<int>(node.rotation.type);
			if (ImGui::Combo("Rotation Type", &rotationType, rotationTypeStrings, IM_ARRAYSIZE(rotationTypeStrings)))
			{
				node.rotation.type = static_cast<uint32_t>(rotationType);
			}
			// Rotation
			EFFECT_SRT_TYPE type = static_cast<EFFECT_SRT_TYPE>(node.rotation.type);
			switch (type)
			{
			case EFFECT_SRT_TYPE::SRT_TYPE_STANDARD:
				ImGui::DragFloat3("回転", &node.rotation.value.x, 0.1f);
				break;
			case EFFECT_SRT_TYPE::SRT_TYPE_PVA:
				DragRandfloat3("回転", &node.rotation.pva.value, 0.1f, 0.0f, 0.0f);
				DragRandfloat3("速度", &node.rotation.pva.velocity, 0.1f, 0.0f, 0.0f);
				DragRandfloat3("加速度", &node.rotation.pva.acceleration, 0.1f, 0.0f, 0.0f);
				break;
			case EFFECT_SRT_TYPE::SRT_TYPE_EASING:
				break;
			default:
				break;
			}
			// End
			ImGui::EndTabItem();
		}
		// Scale
		if (ImGui::BeginTabItem("スケール"))
		{
			// ScaleType
			static const char* scaleTypeStrings[] = {
				"Standard",     // ScaleStandard
				"PVA",          // ScalePVA
				//"Easing",       // ScaleEasing
				//"FCurve",       // ScaleFCurve
				//"NURBS",        // ScaleNURBS
				//"CameraOffset", // ScaleCameraOffset
			};
			int scaleType = static_cast<int>(node.scale.type);
			if (ImGui::Combo("Scale Type", &scaleType, scaleTypeStrings, IM_ARRAYSIZE(scaleTypeStrings)))
			{
				node.scale.type = static_cast<uint32_t>(scaleType);
			}
			// Scale
			EFFECT_SRT_TYPE type = static_cast<EFFECT_SRT_TYPE>(node.scale.type);
			switch (type)
			{
			case EFFECT_SRT_TYPE::SRT_TYPE_STANDARD:
				ImGui::DragFloat3("スケール", &node.scale.value.x, 0.1f);
				break;
			case EFFECT_SRT_TYPE::SRT_TYPE_PVA:
				DragRandfloat3("スケール", &node.scale.pva.value, 0.1f, 0.0f, 0.0f);
				DragRandfloat3("速度", &node.scale.pva.velocity, 0.1f, 0.0f, 0.0f);
				DragRandfloat3("加速度", &node.scale.pva.acceleration, 0.1f, 0.0f, 0.0f);
				break;
			case EFFECT_SRT_TYPE::SRT_TYPE_EASING:
				break;
			default:
				break;
			}
			// End
			ImGui::EndTabItem();
		}
		// DrawCommon
		if (ImGui::BeginTabItem("描画共通"))
		{
			ImGui::Text("Drop Texture Here");
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Texture"))
				{
					// ドロップされたテクスチャのIDを取得
					const char* textureName = static_cast<const char*>(payload->Data);
					// テクスチャIDを取得
					node.drawCommon.textureID = m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureID(ConvertString(textureName));
					// テクスチャ名を取得
					node.textureName = ConvertString(textureName);
				}
				ImGui::EndDragDropTarget();
			}
			// テクスチャ表示
			if (node.drawCommon.textureID != 0)
			{
				ImTextureID textureID = (ImTextureID)m_EngineCommand->GetTextureHandle(node.textureName).ptr;
				ImGui::Image(textureID, ImVec2(64, 64));
				std::string label = ConvertString(node.textureName);
				ImGui::Text("%s", label.c_str());
				if (ImGui::Button("Delete Texture"))
				{
					node.drawCommon.textureID = 0;
					node.textureName = L"";
				}
			}
			// End
			ImGui::EndTabItem();
		}
		// Draw
		if (ImGui::BeginTabItem("描画"))
		{
			// MeshType
			static const char* meshTypeStrings[] = {
				"Node",     // MeshNode
				"Sprite",  // MeshSprite
				"Ribbon",  // MeshRibbon
				"Trail",   // MeshTrail
				"Ring",    // MeshRing
			};
			int meshType = static_cast<int>(node.draw.meshType);
			int prevMeshType = meshType;

			if (ImGui::Combo("Mesh Type", &meshType, meshTypeStrings, IM_ARRAYSIZE(meshTypeStrings)))
			{
				if (meshType != prevMeshType)
				{
					// 値が変わったときだけ行う処理
					EFFECT_MESH_TYPE postType = static_cast<EFFECT_MESH_TYPE>(meshType);
					EFFECT_MESH_TYPE preType = static_cast<EFFECT_MESH_TYPE>(prevMeshType);
					switch (postType)
					{
					case EFFECT_MESH_TYPE::NONE:
						switch (preType)
						{
						case EFFECT_MESH_TYPE::NONE:
							// 何もしない
							break;
						case EFFECT_MESH_TYPE::SPRITE:
							m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::EffectSpriteInt)->RemoveMapID(node.draw.meshDataIndex);
							break;
						case EFFECT_MESH_TYPE::RIBBON:
							break;
						case EFFECT_MESH_TYPE::TRAIL:
							break;
						case EFFECT_MESH_TYPE::RING:
							m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::EffectRingInt)->RemoveMapID(node.draw.meshDataIndex);
							break;
						case EFFECT_MESH_TYPE::MODEL:
							break;
						case EFFECT_MESH_TYPE::CUBE:
							break;
						case EFFECT_MESH_TYPE::SPHERE:
							break;
						default:
							break;
						}
						break;
					case EFFECT_MESH_TYPE::SPRITE:
						switch (preType)
						{
						case EFFECT_MESH_TYPE::NONE:
							// 何もしない
							break;
						case EFFECT_MESH_TYPE::SPRITE:
							// 何もしない
							break;
						case EFFECT_MESH_TYPE::RIBBON:
							break;
						case EFFECT_MESH_TYPE::TRAIL:
							break;
						case EFFECT_MESH_TYPE::RING:
							m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::EffectRingInt)->RemoveMapID(node.draw.meshDataIndex);
							break;
						case EFFECT_MESH_TYPE::MODEL:
							break;
						case EFFECT_MESH_TYPE::CUBE:
							break;
						case EFFECT_MESH_TYPE::SPHERE:
							break;
						default:
							break;
						}
						node.draw.meshDataIndex = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::EffectSpriteInt)->GetMapID();
						break;
					case EFFECT_MESH_TYPE::RIBBON:
						break;
					case EFFECT_MESH_TYPE::TRAIL:
						break;
					case EFFECT_MESH_TYPE::RING:
						switch (preType)
						{
						case EFFECT_MESH_TYPE::NONE:
							// 何もしない
							break;
						case EFFECT_MESH_TYPE::SPRITE:
							m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::EffectSpriteInt)->RemoveMapID(node.draw.meshDataIndex);
							break;
						case EFFECT_MESH_TYPE::RIBBON:
							break;
						case EFFECT_MESH_TYPE::TRAIL:
							break;
						case EFFECT_MESH_TYPE::RING:
							// 何もしない
							break;
						case EFFECT_MESH_TYPE::MODEL:
							break;
						case EFFECT_MESH_TYPE::CUBE:
							break;
						case EFFECT_MESH_TYPE::SPHERE:
							break;
						default:
							break;
						}
						node.draw.meshDataIndex = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::EffectRingInt)->GetMapID();
						break;
					case EFFECT_MESH_TYPE::MODEL:
						break;
					case EFFECT_MESH_TYPE::CUBE:
						break;
					case EFFECT_MESH_TYPE::SPHERE:
						break;
					default:
						break;
					}

					node.draw.meshType = static_cast<uint32_t>(meshType);
				}
			}
			// Mesh
			EFFECT_MESH_TYPE type = static_cast<EFFECT_MESH_TYPE>(node.draw.meshType);
			switch (type)
			{
			case EFFECT_MESH_TYPE::NONE:
				// 何もしない
				break;
			case EFFECT_MESH_TYPE::SPRITE:
			{
				// 全体色
				static const char* colorTypeStrings[] = {
					"Standard",     // ColorStandard
					"Random",       // ColorRandom
					"Easing",       // ColorEasing
					//"FCurve",       // ColorFCurve
					//"NURBS",        // ColorNURBS
				};
				EffectSprite* sprite = std::get_if<EffectSprite>(&node.drawMesh);
				int colorType = static_cast<int>(sprite->colorType);
				if (ImGui::Combo("Color Type", &colorType, colorTypeStrings, IM_ARRAYSIZE(colorTypeStrings)))
				{
					sprite->colorType = static_cast<uint32_t>(colorType);
				}
				// 色
				COLOR_TYPE e_type = static_cast<COLOR_TYPE>(sprite->colorType);
				switch (e_type)
				{
				case COLOR_TYPE::CONSTANT:
				{
					ImGui::ColorEdit4("Color", &sprite->color.r);
					// 描画順
					// 配置方法
					static const char* placementStrings[] = {
						"ビルボード",     // Billboard
						"Y軸固定ビルボード", // BillboardY
						"Z軸回転ビルボード", // BillboardZ
						"固定", // Fixed
					};
					int placement = static_cast<int>(sprite->placement);
					if (ImGui::Combo("Placement", &placement, placementStrings, IM_ARRAYSIZE(placementStrings)))
					{
						sprite->placement = static_cast<uint32_t>(placement);
					}
					// 頂点色
					// 頂点座標
					static const char* vertexPositionTypeStrings[] = {
						"Standard",     // VertexPositionStandard
						"Constant",     // VertexPositionConstant
					};
					int vertexPositionType = static_cast<int>(sprite->vertexPositionType);
					if (ImGui::Combo("Vertex Position Type", &vertexPositionType, vertexPositionTypeStrings, IM_ARRAYSIZE(vertexPositionTypeStrings)))
					{
						sprite->vertexPositionType = static_cast<uint32_t>(vertexPositionType);
					}
					// 頂点座標
					VERTEX_POSITION_TYPE vpType = static_cast<VERTEX_POSITION_TYPE>(sprite->vertexPositionType);
					switch (vpType)
					{
					case VERTEX_POSITION_TYPE::STANDARD:
						// 何もしない
						break;
					case VERTEX_POSITION_TYPE::CONSTANT:
						ImGui::DragFloat2("左下座標", &sprite->vertexPosition.leftBottom.x, 0.1f);
						ImGui::DragFloat2("右下座標", &sprite->vertexPosition.rightBottom.x, 0.1f);
						ImGui::DragFloat2("左上座標", &sprite->vertexPosition.leftTop.x, 0.1f);
						ImGui::DragFloat2("右上座標", &sprite->vertexPosition.rightTop.x, 0.1f);
						break;
					default:
						break;
					}
					break;
				}
				case COLOR_TYPE::RANDOM:
					break;
				case COLOR_TYPE::EASING:
					break;
				default:
					break;
				}
				break;
			}
			case EFFECT_MESH_TYPE::RIBBON:
				break;
			case EFFECT_MESH_TYPE::TRAIL:
				break;
			case EFFECT_MESH_TYPE::RING:

				break;
			case EFFECT_MESH_TYPE::MODEL:
				break;
			case EFFECT_MESH_TYPE::CUBE:
				break;
			case EFFECT_MESH_TYPE::SPHERE:
				break;
			default:
				break;
			}
			// End
			ImGui::EndTabItem();
		}


		// End
		ImGui::EndTabBar();
	}

	ImGui::Text("Effect Editor");
	ImGui::End();
}

void EffectEditor::ControlWindow()
{
	ImGui::Begin("EffectControl");

	if (!m_EditorManager->GetEffectEntity().has_value())
	{
		ImGui::Text("No Root");
		ImGui::End();
		return;
	} else
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

	ImGui::Text("time : %f", effect->root.second.time.elapsedTime);
	ImGui::Text("maxTime : %f", effect->root.second.time.duration);

    // 中央に配置するための計算
    float toolbarWidth = 0.0f;
    float buttonSize = 28.0f; // ボタンサイズ
    float buttonSpacing = 8.0f; // ボタン間のスペース
    int buttonCount = 3;
    toolbarWidth = buttonCount * buttonSize + (buttonCount - 1) * buttonSpacing;

    // ウィンドウ中央位置に移動
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float startX = (availableWidth - toolbarWidth) * 0.5f;
    ImGui::SetCursorPosX(startX);

    // ゲーム実行中なら
    if (effect->isRun)
    {
        // ボタンを描画（中央に並べる）
        if (ImGui::Button(ICON_FA_PAUSE, ImVec2(buttonSize, buttonSize)))
        {
            /* Pause */
            effect->isRun = false;
        }
    } else // 実行中じゃないなら
    {
        // ボタンを描画（中央に並べる）
        if (ImGui::Button(ICON_FA_PLAY, ImVec2(buttonSize, buttonSize)))
        {
            // Play
            effect->isRun = true;
        }
    }
	if (ImGui::Button("1Frame"))
	{
		effect->root.second.time.elapsedTime++;
	}
	int time = static_cast<int>(effect->root.second.time.elapsedTime);
	ImGui::SliderInt("Time", &time, 0, static_cast<int>(effect->root.second.time.duration));
	effect->root.second.time.elapsedTime = static_cast<float>(time);
	ImGui::End();
}

bool EffectEditor::DragRandfloat3(const char* label, Randfloat3* v, float v_speed, float v_min, float v_max)
{
    ImGui::PushID(label); // 識別子をプッシュ（同じ名前のコントロールが競合しないようにする）

	ImGui::Text("%s", label); // ラベルを表示

    float item_width = ImGui::CalcItemWidth(); // 現在の項目幅を取得
    float single_item_width = (item_width - ImGui::GetStyle().ItemSpacing.x * 2) / 3; // 3分割

    //--中央値--//
    // X
    ImGui::PushItemWidth(single_item_width);
	bool mx_changed = ImGui::DragFloat("X##MX", &v->x.median, v_speed, v_min, v_max,":%.3f");
    ImGui::PopItemWidth();
    ImGui::SameLine(); // 横並び
	// Y
	ImGui::PushItemWidth(single_item_width);
	bool my_changed = ImGui::DragFloat("Y##MY", &v->y.median, v_speed, v_min, v_max, ":%.3f");
	ImGui::PopItemWidth();
	ImGui::SameLine(); // 横並び
	// Z
	ImGui::PushItemWidth(single_item_width);
	bool mz_changed = ImGui::DragFloat("Z##MZ", &v->z.median, v_speed, v_min, v_max, ":%.3f");
	ImGui::PopItemWidth();
    ImGui::SameLine();        // ラベルとスライダーを横並びにする
    ImGui::Text("中心"); // ラベルを表示

    //--振幅--//
	// X
	ImGui::PushItemWidth(single_item_width);
	bool ax_changed = ImGui::DragFloat("X##AX", &v->x.amplitude, v_speed, 0.0f, v_max, ":%.3f");
	ImGui::PopItemWidth();
	ImGui::SameLine(); // 横並び
	// Y
	ImGui::PushItemWidth(single_item_width);
	bool ay_changed = ImGui::DragFloat("Y##AY", &v->y.amplitude, v_speed, 0.0f, v_max, ":%.3f");
	ImGui::PopItemWidth();
	ImGui::SameLine(); // 横並び
	// Z
	ImGui::PushItemWidth(single_item_width);
	bool az_changed = ImGui::DragFloat("Z##AZ", &v->z.amplitude, v_speed, 0.0f, v_max, ":%.3f");
	ImGui::PopItemWidth();
	ImGui::SameLine();        // ラベルとスライダーを横並びにする
	ImGui::Text("振幅"); // ラベルを表示

    ImGui::PopID(); // 識別子をポップ

	return mx_changed || my_changed || mz_changed ||
		ax_changed || ay_changed || az_changed;
}
