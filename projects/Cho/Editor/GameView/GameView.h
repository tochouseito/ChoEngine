#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class GameView : public BaseEditor
{
public:
	GameView(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~GameView()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

