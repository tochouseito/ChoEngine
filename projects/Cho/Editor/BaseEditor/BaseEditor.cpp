#include "pch.h"
#include "BaseEditor.h"
#include "Editor/EditorManager/EditorManager.h"

BaseEditor::BaseEditor(EditorManager* editorManager) :
	m_EditorManager(editorManager)
{
	m_EngineCommand=m_EditorManager->GetEngineCommand();
}
