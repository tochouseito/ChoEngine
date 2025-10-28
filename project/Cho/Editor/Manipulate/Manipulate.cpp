#include "pch.h"
#include "Manipulate.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "GameCore/GameCore.h"

void Manipulate::Initialize()
{
}

void Manipulate::Update()
{
	// 編集対象のオブジェクトを取得
	GameObject* target = m_EditorManager->GetSelectedGameObject();
	if (target == nullptr) return;
	// Transformコンポーネントを取得
	TransformComponent* transform = m_EngineCommand->GetGameCore()->GetECSManager()->GetComponent<TransformComponent>(target->GetHandle().entity);
	if (transform == nullptr) return;

	CameraComponent& camera = m_dCamera->GetCameraComponent();
	float4x4 view = camera.viewMatrix;
	float4x4 projection = camera.projectionMatrix;

	// 変換
	float viewOut[16]{};
	float projectionOut[16]{};
	float objectOut[16]{};
	view.ToArray16(viewOut);
	projection.ToArray16(projectionOut);
	transform->matWorld.ToArray16(objectOut);

	// ImGuizmoの描画設定
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(m_ContentPos.x, m_ContentPos.y, m_ContentSize.x, m_ContentSize.y);

	// 操作タイプ（移動・回転・スケールを切り替えるフラグ）
	static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	static ImGuizmo::MODE mode = ImGuizmo::WORLD;

	// ③ 操作タイプ切り替えボタンを「画像の上」に重ねて描画
	//    - SetCursorScreenPos で好きな場所にカーソルを移動してからボタンを置く
	const float btnSize = 24.0f;
	// 左上に少しだけマージンを空けて並べる
	ImGui::SetCursorScreenPos({ m_ContentPos.x + 10, m_ContentPos.y + 10 });
	if (ImGui::Button("T", ImVec2(btnSize, btnSize))) operation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::Button("R", ImVec2(btnSize, btnSize))) operation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::Button("S", ImVec2(btnSize, btnSize))) operation = ImGuizmo::SCALE;

	// ギズモ操作
	bool manipulated = ImGuizmo::Manipulate(
		viewOut, projectionOut,
		operation, mode,
		objectOut, nullptr // nullptrにすればdelta行列不要
	);

	// モデル行列が変更されたら、オブジェクトに反映
	if (manipulated)
	{
		// model (float[16]) → XMMATRIX へ変換
		float4x4 newMatrix = float4x4::FromArray16(objectOut);
		// 分解して各プロパティを更新
		Theatria::Math::SRT srt = Theatria::Math::DecomposeMatrix(newMatrix);
		transform->position = srt.translation;
		transform->degrees = Theatria::Math::RadiansToDegrees(srt.rotationEuler);
		transform->scale = srt.scale;
	}
}

void Manipulate::Window()
{
}
