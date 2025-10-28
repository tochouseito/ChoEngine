#pragma once
#include "Core/Utility/Components.h"
#include "Core/Utility/CompBufferData.h"
#include "SDK/DirectX/DirectX12/GpuBuffer/GpuBuffer.h"
class EditorManager;
class DebugCamera
{
	friend class Manipulate;
public:
	DebugCamera(EditorManager* editorManager)
		:m_pEditorManager(editorManager)
	{

	}
	~DebugCamera()
	{

	}
	void Initialize();
	void Update();
	void UpdateMatrix();
	void TransferMatrix();
	void SetAspect(const float& aspect) { m_CameraComponent.aspectRatio = aspect; }
private:
	TransformComponent& GetTransformComponent() { return m_TransformComponent; }
	CameraComponent& GetCameraComponent() { return m_CameraComponent; }

	EditorManager* m_pEditorManager = nullptr;
	TransformComponent m_TransformComponent;
	CameraComponent m_CameraComponent;
	//BUFFER_DATA_TF m_TransformData;
	BUFFER_DATA_VIEWPROJECTION m_ViewProjectionData;
	// バッファ
	//ConstantBuffer<BUFFER_DATA_TF>* m_pTransformBuffer = nullptr;
	ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>* m_pCameraBuffer = nullptr;

	// カーソルを動かすときの感度
	const float mouseSensitivity = 0.1f;
	// カメラの移動速度
	const float moveSpeed = 0.1f;

	float2 mousePos;
	float2 deltaMousePos;
	float2 preMousePos;
};

