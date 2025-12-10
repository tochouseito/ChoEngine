#include "pch.h"
#include "CommandContext.h"
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;

void CommandContext::Create(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	HRESULT hr = {};
	// コマンドアロケータを生成する
	hr = device->CreateCommandAllocator(
		type,
		IID_PPV_ARGS(&m_CommandAllocator)
	);
	// コマンドアロケータの生成がうまくいかなかったので起動できない
	Log::Write(LogLevel::Assert, "CommandAllocator created.", hr);

	// コマンドリストを生成する
	hr = device->CreateCommandList(
		0,
		type,
		m_CommandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_CommandList)
	);
	// コマンドリストの生成がうまくいかなかったので起動できない
	Log::Write(LogLevel::Assert, "CommandList created.", hr);

	m_Type = type;
	m_CommandList->Close();  // 初期状態で閉じておく
}

CommandContext::~CommandContext()
{
}

void CommandContext::Reset()
{
	HRESULT hr = {};
	// コマンドアロケータをリセットする
	hr = m_CommandAllocator->Reset();
	// コマンドアロケータのリセットがうまくいかなかったので起動できない
	//Log::Write(LogLevel::Assert, "CommandAllocator reset.", hr);
	// コマンドリストをリセットする
	hr = m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
	// コマンドリストのリセットがうまくいかなかったので起動できない
	//Log::Write(LogLevel::Assert, "CommandList reset.", hr);
}

void CommandContext::Close()
{
	HRESULT hr = {};
	// コマンドリストを閉じる
	hr = m_CommandList->Close();
	// コマンドリストのクローズがうまくいかなかったので起動できない
	//Log::Write(LogLevel::Assert, "CommandList closed.", hr);
}

void CommandContext::Flush()
{
}

void CommandContext::SetDescriptorHeap(ID3D12DescriptorHeap* heap)
{
	// ディスクリプタヒープを設定する
	ID3D12DescriptorHeap* heaps[] = { heap };
	// コマンドリストにディスクリプタヒープを設定する
	m_CommandList->SetDescriptorHeaps(_countof(heaps), heaps);
}

void CommandContext::BarrierTransition(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
{
	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = pResource;
	// 遷移前（現在）のResourceState
	barrier.Transition.StateBefore = Before;
	// 遷移後のResourceState
	barrier.Transition.StateAfter = After;
    // 全てのミップマップに対してバリアを張る
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	// TransitionBarrierを張る
	m_CommandList->ResourceBarrier(1, &barrier);
}

void CommandContext::BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE Type, D3D12_RESOURCE_BARRIER_FLAGS Flags, ID3D12Resource* pResource)
{
	// 並列処理の阻止
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = Type;
	barrier.Flags = Flags;
	barrier.UAV.pResource = pResource;
	// UAVバリアを張る
	m_CommandList->ResourceBarrier(1, &barrier);
}

void CommandContext::ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
	// リソースバリアの設定
	m_CommandList->ResourceBarrier(NumBarriers, pBarriers);
}

void CommandContext::SetRenderTarget(ColorBuffer* rtv, DepthBuffer* depth)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	// リソースステートのチェック
	CheckResourceStateTransition(rtv, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (depth)
	{
		dsvHandle = depth->GetDSVCpuHandle();
		CheckResourceStateTransition(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	// レンダーターゲットビューの設定
	rtvHandle = rtv->GetRTVCpuHandle();
	m_CommandList->OMSetRenderTargets(1, &rtvHandle, false, depth ? &dsvHandle : nullptr);
}

void CommandContext::SetRenderTarget(SwapChainBuffer* swapChainBuffer)
{
	// リソースステートのチェック
	CheckResourceStateTransition(swapChainBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// レンダーターゲットビューの設定
	m_CommandList->OMSetRenderTargets(1, &swapChainBuffer->m_RTVCpuHandle, false, nullptr);
}

void CommandContext::ClearRenderTarget(ColorBuffer* rt)
{
	// リソースステートのチェック
	CheckResourceStateTransition(rt, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// 指定した色で画面全体をクリアする
	m_CommandList->ClearRenderTargetView(rt->GetRTVCpuHandle(), kClearColor, 0, nullptr);
}

void CommandContext::ClearRenderTarget(SwapChainBuffer* swapChainBuffer)
{
	// リソースステートのチェック
	CheckResourceStateTransition(swapChainBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// 指定した色で画面全体をクリアする
	m_CommandList->ClearRenderTargetView(swapChainBuffer->m_RTVCpuHandle, kClearColor, 0, nullptr);
}

void CommandContext::ClearDepthStencil(DepthBuffer* depth)
{
	// リソースステートのチェック
	CheckResourceStateTransition(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	// 深度ステンシルビューのクリア
	m_CommandList->ClearDepthStencilView(depth->GetDSVCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void CommandContext::ClearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, ID3D12Resource* pResource,const UINT* value,UINT numRects, const D3D12_RECT* pRects)
{
	// UAVのクリア
	m_CommandList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pResource, value, numRects, pRects);
}

void CommandContext::SetViewport(const D3D12_VIEWPORT& viewport)
{
	// ビューポートの設定
	m_CommandList->RSSetViewports(1, &viewport);
}

void CommandContext::SetScissorRect(const D3D12_RECT& rect)
{
	// シザーレクトの設定
	m_CommandList->RSSetScissorRects(1, &rect);
}

void CommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	// プリミティブトポロジーの設定
	m_CommandList->IASetPrimitiveTopology(topology);
}

void CommandContext::SetGraphicsPipelineState(ID3D12PipelineState* pso)
{
	// パイプラインステートの設定
	m_CommandList->SetPipelineState(pso);
}

void CommandContext::SetComputePipelineState(ID3D12PipelineState* pso)
{
	// コンピュートパイプラインステートの設定
	m_CommandList->SetPipelineState(pso);
}

void CommandContext::SetGraphicsRootSignature(ID3D12RootSignature* rootSignature)
{
	// ルートシグネチャの設定
	m_CommandList->SetGraphicsRootSignature(rootSignature);
}

void CommandContext::SetComputeRootSignature(ID3D12RootSignature* rootSignature)
{
	// コンピュートルートシグネチャの設定
	m_CommandList->SetComputeRootSignature(rootSignature);
}

void CommandContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW* pViews)
{
	// 頂点バッファの設定
	m_CommandList->IASetVertexBuffers(StartSlot, Count, pViews);
}

void CommandContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* pView)
{
	// インデックスバッファの設定
	m_CommandList->IASetIndexBuffer(pView);
}

void CommandContext::SetGraphicsRootConstantBufferView(UINT RootParameterIndex, GpuResource* pResource)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// ルート定数バッファビューの設定
	m_CommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, pResource->GetResource()->GetGPUVirtualAddress());
}

void CommandContext::SetComputeRootConstantBufferView(UINT RootParameterIndex, GpuResource* pResource)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// コンピュートルート定数バッファビューの設定
	m_CommandList->SetComputeRootConstantBufferView(RootParameterIndex, pResource->GetResource()->GetGPUVirtualAddress());
}

void CommandContext::SetGraphicsRootShaderResourceView(UINT RootParameterIndex, GpuResource* pResource)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// ルートシェーダリソースビューの設定
	m_CommandList->SetGraphicsRootShaderResourceView(RootParameterIndex, pResource->GetResource()->GetGPUVirtualAddress());
}

void CommandContext::SetComputeRootShaderResourceView(UINT RootParameterIndex, GpuResource* pResource)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// コンピュートルートシェーダリソースビューの設定
	m_CommandList->SetComputeRootShaderResourceView(RootParameterIndex, pResource->GetResource()->GetGPUVirtualAddress());
}

void CommandContext::SetGraphicsRootUnorderedAccessView(UINT RootParameterIndex, GpuResource* pResource)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// ルートシェーダリソースビューの設定
	m_CommandList->SetGraphicsRootUnorderedAccessView(RootParameterIndex, pResource->GetResource()->GetGPUVirtualAddress());
}

void CommandContext::SetComputeRootUnorderedAccessView(UINT RootParameterIndex, GpuResource* pResource)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// コンピュートルートシェーダリソースビューの設定
	m_CommandList->SetComputeRootUnorderedAccessView(RootParameterIndex, pResource->GetResource()->GetGPUVirtualAddress());
}

void CommandContext::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, GpuResource* pResource, ViewType viewType)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// ルートディスクリプタテーブルの設定
	switch (viewType)
	{
	case ViewType::ConstantBufferView:
		break;
	case ViewType::ShaderResourceView:
		m_CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, pResource->GetSRVGpuHandle());
		break;
	case ViewType::UnorderedAccessView:
		m_CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, pResource->GetUAVGpuHandle());
		break;
	case ViewType::RenderTargetView:
		break;
	case ViewType::DepthStencilView:
		break;
	default:
		break;
	}
}

void CommandContext::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, ID3D12DescriptorHeap* pDescHeap)
{
	// ルートディスクリプタテーブルの設定
	m_CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, pDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void CommandContext::SetComputeRootDescriptorTable(UINT RootParameterIndex, GpuResource* pResource, ViewType viewType)
{
	// リソースステートのチェック
	CheckResourceStateTransition(pResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// コンピュートルートディスクリプタテーブルの設定
	switch (viewType)
	{
	case ViewType::ConstantBufferView:
		break;
	case ViewType::ShaderResourceView:
		m_CommandList->SetComputeRootDescriptorTable(RootParameterIndex, pResource->GetSRVGpuHandle());
		break;
	case ViewType::UnorderedAccessView:
		m_CommandList->SetComputeRootDescriptorTable(RootParameterIndex, pResource->GetUAVGpuHandle());
		break;
	case ViewType::RenderTargetView:
		break;
	case ViewType::DepthStencilView:
		break;
	default:
		break;
	}
}

void CommandContext::SetComputeRootDescriptorTable(UINT RootParameterIndex, ID3D12DescriptorHeap* pDescHeap)
{
	// コンピュートルートディスクリプタテーブルの設定
	m_CommandList->SetComputeRootDescriptorTable(RootParameterIndex, pDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void CommandContext::CopyBufferRegion(GpuBuffer* pDstBuffer, UINT DstOffset, GpuBuffer* pSrcBuffer, UINT SrcOffset, UINT NumBytes)
{
	// コピー元リソースステートのチェック
	CheckResourceStateTransition(pSrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	// コピー先リソースステートのチェック
	CheckResourceStateTransition(pDstBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	// バッファのコピー
	m_CommandList->CopyBufferRegion(pDstBuffer->GetResource(), DstOffset, pSrcBuffer->GetResource(), SrcOffset, NumBytes);
}

void CommandContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	// 描画
	m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CommandContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	// インデックス付き描画
	m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CommandContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	// コンピュートシェーダのディスパッチ
	m_CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void CommandContext::ExecuteIndirect(ID3D12CommandSignature* pCommandSignature, UINT MaxCommandCount, GpuResource* pArgumentResource, UINT ArgumentBufferOffset, ID3D12Resource* pCountBuffer, UINT CountBufferOffset)
{
	// 引数バッファのリソースステートのチェック
	CheckResourceStateTransition(pArgumentResource, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	// 間接コマンドの実行
	m_CommandList->ExecuteIndirect(pCommandSignature, MaxCommandCount, pArgumentResource->GetResource(), ArgumentBufferOffset, pCountBuffer, CountBufferOffset);
}

void CommandContext::CheckResourceStateTransition(GpuResource* pResource, D3D12_RESOURCE_STATES checkState)
{
	// UploadHeapはState遷移しない
	if (pResource->GetHeapType() == D3D12_HEAP_TYPE_UPLOAD) { return; }
	// リソースステートのチェック
	if(pResource->GetResourceState() == checkState)
	{
		// 問題なし
		return;
	}
	// リソースステートの遷移
	BarrierTransition(pResource->GetResource(), pResource->GetResourceState(), checkState);
	// リソースステートの更新
	pResource->SetResourceState(checkState);
}

void CommandContext::CheckResourceStateTransition(SwapChainBuffer* swapChainBuffer, D3D12_RESOURCE_STATES checkState)
{
	// リソースステートのチェック
	if (swapChainBuffer->m_ResourceState == checkState)
	{
		// 問題なし
		return;
	}
	// リソースステートの遷移
	BarrierTransition(swapChainBuffer->pResource.Get(), swapChainBuffer->m_ResourceState, checkState);
	// リソースステートの更新
	swapChainBuffer->m_ResourceState = checkState;
}

GraphicsContext::GraphicsContext(ID3D12Device* device)
{
	Create(device, D3D12_COMMAND_LIST_TYPE_DIRECT);	
}

GraphicsContext::~GraphicsContext()
{
}

ComputeContext::ComputeContext(ID3D12Device* device)
{
	Create(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

ComputeContext::~ComputeContext()
{
}
