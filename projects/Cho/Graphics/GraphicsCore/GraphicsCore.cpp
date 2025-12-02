#include "pch.h"
#include "GraphicsCore.h"
#include "SDK/DirectX/DirectX12/CommandContext/CommandContext.h"
#include "SDK/DirectX/DirectX12/CommandManager/CommandManager.h"

ID3D12CommandQueue* GraphicsCore::GetCommandQueue(const QueueType& type) const
{
	return m_CommandManager->GetCommandQueue(type);
}
