#include "pch.h"
#include "MultiSystems.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "GameCore/Marionnette/Marionnette.h"
#include "Platform/FileSystem/FileSystem.h"
#include "Core/ChoLog/ChoLog.h"

void LineRendererSystem::TransferMatrix(LineRendererComponent& lineRenderer)
{
	// 転送
	uint32_t index = lineRenderer.mapID.value() * 2;
	{// 始点
		BUFFER_DATA_LINE bufferData;
		bufferData.position = lineRenderer.line.start;
		bufferData.color = lineRenderer.line.color;
		m_pIntegrationBuffer->UpdateData(bufferData, index);
	}
	{// 終点
		BUFFER_DATA_LINE bufferData;
		bufferData.position = lineRenderer.line.end;
		bufferData.color = lineRenderer.line.color;
		m_pIntegrationBuffer->UpdateData(bufferData, index + 1);
	}
}
