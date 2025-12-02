#pragma once
#include "GameCore/ECS/ECSManager.h"
#include "SDK/DirectX/DirectX12/GpuBuffer/GpuBuffer.h"
#include "Core/Utility/CompBufferData.h"
#include "Resources/ResourceManager/ResourceManager.h"

class ResourceManager;

class LineRendererSystem : public ECSManager::MultiComponentSystem<LineRendererComponent>
{
	friend class GameCore;
	public:
		LineRendererSystem() : ECSManager::MultiComponentSystem<LineRendererComponent>(
			[&](Entity e, std::vector<LineRendererComponent>& lines)
			{
				for (LineRendererComponent& line : lines)
				{
					UpdateComponent(e, line);
				}
			},
			[&](Entity e, std::vector<LineRendererComponent>& lines)
			{
				for (LineRendererComponent& line : lines)
				{
					InitializeComponent(e, line);
				}
			},
			[&](Entity e, std::vector<LineRendererComponent>& lines)
			{
				for (LineRendererComponent& line : lines)
				{
					FinalizeComponent(e, line);
				}
			})
	{
	}
	~LineRendererSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, [[maybe_unused]] LineRendererComponent& line)
	{
	}
	void UpdateComponent([[maybe_unused]] Entity e, LineRendererComponent& line)
	{
		// 転送
		TransferMatrix(line);
	}
	void TransferMatrix(LineRendererComponent& lineRenderer);
	void FinalizeComponent([[maybe_unused]] Entity e, [[maybe_unused]] LineRendererComponent& line)
	{
	}

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	void SetBuffer(VertexBuffer<BUFFER_DATA_LINE>* buffer)
	{
		m_pIntegrationBuffer = buffer;
	}

	ResourceManager* m_pResourceManager = nullptr;
	VertexBuffer<BUFFER_DATA_LINE>* m_pIntegrationBuffer = nullptr;
};


