#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class GameObject;
class Inspector : public BaseEditor
{
public:
	Inspector(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~Inspector()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
private:
	// コンポーネントの表示
	void ComponentsView(GameObject* object);
	// TransformComponentの表示
	void TransformComponentView(GameObject* object);
	// MeshFilterComponentの表示
	void MeshFilterComponentView(GameObject* object);
	// MeshRendererComponentの表示
	void MeshRendererComponentView(GameObject* object);
	// CameraComponentの表示
	void CameraComponentView(GameObject* object);
	// UISpriteComponentの表示
	void UISpriteComponentView(GameObject* object);
	// MaterialComponentの表示
	void MaterialComponentView(GameObject* object);
	// ScriptComponentの表示
	void ScriptComponentView(GameObject* object);
	// LineRendererComponentの表示
	void LineRendererComponentView(GameObject* object);
	// Rigidbody2DComponentの表示
	void Rigidbody2DComponentView(GameObject* object);
	// BoxCollider2DComponentの表示
	void BoxCollider2DComponentView(GameObject* object);
	// Rigidbody3DComponentの表示
	void Rigidbody3DComponentView(GameObject* object);
	// EmitterComponentの表示
	void EmitterComponentView(GameObject* object);
	// ParticleComponentの表示
	void ParticleComponentView(GameObject* object);
	// LightComponentの表示
	void LightComponentView(GameObject* object);
	// AudioComponentの表示
	void AudioComponentView(GameObject* object);
	// AnimtationComponentの表示
	void AnimationComponentView(GameObject* object);
	// コンポーネントの追加
	void AddComponent(GameObject* object);
};

