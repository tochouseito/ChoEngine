#include "pch.h"
#include "CommandManager.h"
#include "Core/ThreadManager/ThreadManager.h"

CommandManager::CommandManager(ID3D12Device8* device, ThreadManager* threadManager):
    m_Device(device),
    m_ThreadManager(threadManager)
{
	CreateQueueContexts(device);// デバイスを受け取り、キューコンテキストを作成する
	Initialize();// 初期化
}

CommandManager::~CommandManager()
{
}

void CommandManager::Initialize()
{
	m_CommandPool = std::make_unique<CommandPool>(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ThreadManager->GetThreadCount());
}

void CommandManager::Shutdown()
{
}

ID3D12CommandQueue* CommandManager::GetCommandQueue(QueueType type)
{
	return m_QueueContexts[type]->GetCommandQueue();
}

uint64_t CommandManager::ExecuteCommandList(ID3D12GraphicsCommandList6* commandList, QueueType type)
{
	switch (type)
	{
	case Graphics:
		m_QueueContexts[Graphics]->ExecuteCommandLists(commandList);
		break;
	case Compute:
		m_QueueContexts[Compute]->ExecuteCommandLists(commandList);
		break;
	case Copy:
		m_QueueContexts[Copy]->ExecuteCommandLists(commandList);
		break;
	default:
		break;
	}

	return 0;
}

void CommandManager::Signal(QueueType type)
{
	switch (type)
	{
	case Graphics:
		m_QueueContexts[Graphics]->Signal();
		break;
	case Compute:
		m_QueueContexts[Compute]->Signal();
		break;
	case Copy:
		m_QueueContexts[Copy]->Signal();
		break;
	default:
		break;
	}
}

void CommandManager::WaitForFence(QueueType type)
{
	switch (type)
	{
	case Graphics:
		m_QueueContexts[Graphics]->WaitForFence();
		break;
	case Compute:
		m_QueueContexts[Compute]->WaitForFence();
		break;
	case Copy:
		m_QueueContexts[Copy]->WaitForFence();
		break;
	default:
		break;
	}
}

void CommandManager::CreateQueueContexts(ID3D12Device8* device)
{
	for (uint32_t i = 0;i < kQueueTypeCount;++i)
	{
		m_QueueContexts[i] = std::make_unique<QueueContext>(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
}

QueueContext::QueueContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) : m_Device(device)
{
	HRESULT hr = {};
	/*FenceCreate*/
	// 初期値0でFenceを作る
	m_Fence = nullptr;
	hr = device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	Log::Write(LogLevel::Assert, "Fence created.", hr);
	// FenceのSignalを持つためのイベントを作成する
	m_FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	Log::Write(LogLevel::Assert, "Fence event created.", static_cast<bool>(m_FenceEvent != nullptr));

	// コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = type;
	hr = device->CreateCommandQueue(
		&desc,
		IID_PPV_ARGS(&m_CommandQueue)
	);
	Log::Write(LogLevel::Assert, "Command queue created.", hr);
}

QueueContext::~QueueContext()
{
	CloseHandle(m_FenceEvent);
}

void QueueContext::Initialize()
{
}

void QueueContext::Reset()
{
}

void QueueContext::ExecuteCommandLists(ID3D12GraphicsCommandList6* commandList)
{
	ComPtr<ID3D12CommandList> ppCommandLists[] = { commandList };
	m_CommandQueue->ExecuteCommandLists(1, ppCommandLists->GetAddressOf());
}

void QueueContext::Signal()
{
	m_FenceValue++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue);
}

void QueueContext::WaitForFence()
{
	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompletedValueの初期値はFence作成時に渡した初期値
	if (m_Fence->GetCompletedValue() < m_FenceValue)
	{
		// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
		m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
		// イベント待つ
		WaitForSingleObject(m_FenceEvent, INFINITE);
		//if (WaitForSingleObject(m_FenceEvent, 5000) != WAIT_OBJECT_0)
		//{ // 1秒など
		//	HRESULT dr = m_Device->GetDeviceRemovedReason();            // ← ここで呼ぶ
		//	DumpDred(m_Device, dr);
		//	return;
		//}
	}
	//HRESULT dr = m_Device->GetDeviceRemovedReason();
	//if (FAILED(dr))
	//{
	//	DumpDred(m_Device, dr);  // ← ここで DRED 取得
	//	//HandleDeviceLost(dr);
	//	return;
	//}
}

CommandPool::CommandPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, size_t poolSize)
	: m_Device(device), m_Type(type)
{
    poolSize;
	/*for (size_t i = 0; i < poolSize; ++i)
	{
		auto context = std::make_unique<CommandContext>();
		context->Create(device, type);
		m_CommandPool.push(std::move(context));
	}*/
}

CommandPool::~CommandPool()
{
}

CommandContext* CommandPool::GetContext()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_CommandPool.empty())
	{
		auto context = std::make_unique<CommandContext>();
		context->Create(m_Device, m_Type);
		return context.release();
	}
	auto context = std::move(m_CommandPool.front());
	m_CommandPool.pop();
	return context.release();
}

void CommandPool::ReturnContext(CommandContext* context)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CommandPool.push(std::unique_ptr<CommandContext>(context));
}
