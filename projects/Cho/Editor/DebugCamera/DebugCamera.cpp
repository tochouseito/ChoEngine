#include "pch.h"
#include "DebugCamera.h"
#include "Editor/EditorManager/EditorManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "ChoMath.h"
#include "Platform/InputManager/InputManager.h"

void DebugCamera::Initialize()
{
	// バッファの作成
	ResourceManager* resourceManager = m_pEditorManager->GetEngineCommand()->GetResourceManager();
	m_TransformComponent.mapID = resourceManager->CreateConstantBuffer<BUFFER_DATA_TF>();
	m_CameraComponent.bufferIndex = resourceManager->CreateConstantBuffer<BUFFER_DATA_VIEWPROJECTION>();
	//m_pTransformBuffer = dynamic_cast<ConstantBuffer<BUFFER_DATA_TF>*>(resourceManager->GetBuffer<IConstantBuffer>(m_TransformComponent.mapID));
	m_pCameraBuffer = dynamic_cast<ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>*>(resourceManager->GetBuffer<IConstantBuffer>(m_CameraComponent.bufferIndex));
	resourceManager->SetDebugCameraBuffer(m_pCameraBuffer);
	// 初期値
	m_TransformComponent.position = float3(0.0f, 0.0f, -30.0f);
}

void DebugCamera::Update()
{
	// 回転を考慮する
	float4x4 rotationMatrix = Theatria::Math::MakeRotateMatrix(m_TransformComponent.quaternion);
	float3 X = { 1.0f, 0.0f, 0.0f };
	float3 Y = { 0.0f, 1.0f, 0.0f };
	float3 Z = { 0.0f, 0.0f, -1.0f };

	float3 rotatedX = Theatria::Math::TransformPoint(X, rotationMatrix);
	float3 rotatedY = Theatria::Math::TransformPoint(Y, rotationMatrix);
	float3 rotatedZ = Theatria::Math::TransformPoint(Z, rotationMatrix);

	// カメラの操作
	InputManager* inputManager = m_pEditorManager->GetInputManager();
	// 回転
	if (inputManager->IsTriggerMouse(MouseButton::Right))
	{
		// マウスがクリックされたときに現在のマウス位置を保存する
		mousePos = inputManager->GetMouseWindowPosition();
		preMousePos = inputManager->GetMouseWindowPosition();
	}
	if (inputManager->IsPressMouse(MouseButton::Right))
	{
		float2 cur = inputManager->GetMouseWindowPosition();
		float2 delta = cur - preMousePos;
		preMousePos = cur;

		m_TransformComponent.degrees.x = std::clamp(
			m_TransformComponent.degrees.x + delta.y * mouseSensitivity,
			-89.0f, 89.0f     // 真上・真下の反転防止
		);
		m_TransformComponent.degrees.y += delta.x * mouseSensitivity;
	}
	// 位置
	if (inputManager->IsTriggerMouse(MouseButton::Center))
	{
		// マウスがクリックされたときに現在のマウス位置を保存する
		mousePos = inputManager->GetMouseWindowPosition();
		preMousePos = inputManager->GetMouseWindowPosition();
	}
	if (inputManager->IsPressMouse(MouseButton::Center))
	{
		// マウスの移動量を取得
		deltaMousePos.x = inputManager->GetMouseWindowPosition().x - preMousePos.x;
		deltaMousePos.y = inputManager->GetMouseWindowPosition().y - preMousePos.y;
		preMousePos = inputManager->GetMouseWindowPosition();
		// カメラ回転を更新
		m_TransformComponent.position -= rotatedX * deltaMousePos.x * mouseSensitivity;
		m_TransformComponent.position += rotatedY * deltaMousePos.y * mouseSensitivity;
	}
	// 前後移動
	// マウスホイールの移動量を取得する
	int32_t wheelDelta = -inputManager->GetWheel();
	// マウスホイールの移動量に応じてカメラの移動を更新する
	m_TransformComponent.position += rotatedZ * float(wheelDelta) * moveSpeed;
	// デルタマウス位置を初期化
	deltaMousePos.Initialize();

	m_TransformComponent.degrees.z = 0.0f; // Z軸の回転は無効化（カメラの傾き防止）
	UpdateMatrix();
}

void DebugCamera::UpdateMatrix()
{
	// 度数からラジアンに変換
	float3 rad = Theatria::Math::DegreesToRadians(m_TransformComponent.degrees);

	// 差分計算
	//float3 diff = m_TransformComponent.preRot - radians;

	// 各軸のクオータニオンを作成
	//Quaternion qx = ChoMath::MakeRotateAxisAngleQuaternion(float3(1.0f, 0.0f, 0.0f), diff.x);
	//Quaternion qy = ChoMath::MakeRotateAxisAngleQuaternion(float3(0.0f, 1.0f, 0.0f), diff.y);
	//Quaternion qz = ChoMath::MakeRotateAxisAngleQuaternion(float3(0.0f, 0.0f, -1.0f), diff.z);

	Quaternion qYaw = Theatria::Math::MakeRotateAxisAngleQuaternion(float3(0, 1, 0), rad.y);
	Quaternion qPitch = Theatria::Math::MakeRotateAxisAngleQuaternion(float3(1, 0, 0), rad.x);

	// 同時回転を累積
	//m_TransformComponent.rotation = m_TransformComponent.rotation * qx * qy * qz;//*compo.rotation;
	m_TransformComponent.quaternion = qYaw * qPitch;

	// アフィン変換
	m_TransformComponent.matWorld = Theatria::Math::MakeAffineMatrix(Scale(1.0f, 1.0f, 1.0f), m_TransformComponent.quaternion, m_TransformComponent.position);

	// 次のフレーム用に保存する
	m_TransformComponent.prePos = m_TransformComponent.position;
	m_TransformComponent.preRot = rad;

	TransferMatrix();
}

void DebugCamera::TransferMatrix()
{
	// カメラの行列を転送
	m_CameraComponent.viewMatrix = float4x4::Inverse(m_TransformComponent.matWorld);
	m_CameraComponent.projectionMatrix = Theatria::Math::PerspectiveFovMatrix(
		m_CameraComponent.fovAngleY, m_CameraComponent.aspectRatio, m_CameraComponent.nearZ, m_CameraComponent.farZ);
	m_ViewProjectionData.matWorld = m_TransformComponent.matWorld;
	m_ViewProjectionData.view = m_CameraComponent.viewMatrix;
	m_ViewProjectionData.projection = m_CameraComponent.projectionMatrix;
	m_ViewProjectionData.projectionInverse = float4x4::Inverse(m_ViewProjectionData.projection);
	m_ViewProjectionData.cameraPosition = m_TransformComponent.position;
	// 転送
	m_pCameraBuffer->UpdateData(m_ViewProjectionData);
}
