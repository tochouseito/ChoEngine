#pragma once

#include "SDK/DirectX/DirectX12/CommandManager/CommandManager.h"

class ThreadManager;
class GraphicsCore
{
public:
	// Constructor
	GraphicsCore(ID3D12Device8* device, ThreadManager* threadManager) :
		m_CommandManager(std::make_unique<CommandManager>(device, threadManager)),
        m_ThreadManager(threadManager)
	{
	}
	// Destructor
	~GraphicsCore() = default;

	// Getters
	CommandManager* GetCommandManager() const { return m_CommandManager.get(); }
	ID3D12CommandQueue* GetCommandQueue(const QueueType& type)const;
private:
    ThreadManager* m_ThreadManager = nullptr;

	std::unique_ptr<CommandManager> m_CommandManager = nullptr;
};

