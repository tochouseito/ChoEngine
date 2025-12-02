#include "pch.h"
#include "stdafx.h"

void DumpDred(ID3D12Device* device, HRESULT reason)
{
    ComPtr<ID3D12DeviceRemovedExtendedData1> dred1;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dred1)))) return;

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 bc{};
    D3D12_DRED_PAGE_FAULT_OUTPUT1 pf{};
    dred1->GetAutoBreadcrumbsOutput1(&bc);
    dred1->GetPageFaultAllocationOutput1(&pf);

    // ---- ログ例（必要に応じて詳しく）
    OutputDebugStringA("==== DRED Dump ====\n");
    char buf[256];
    sprintf_s(buf, "DeviceRemovedReason=0x%08X\n", (unsigned)reason);
    OutputDebugStringA(buf);

    // 直近のコマンド履歴を辿る（簡略）
    for (auto* n = bc.pHeadAutoBreadcrumbNode; n; n = n->pNext)
    {
        UINT last = 0;
        if (n->pLastBreadcrumbValue) last = *n->pLastBreadcrumbValue; // ★ ポインタをデリファレンス

        // Wide 文字列は %S、UINT は %u でOK
        wprintf(L"CL:%s  Last=%u  Count=%u\n",
            n->pCommandListDebugNameW ? n->pCommandListDebugNameW : L"(null)",
            last,
            n->BreadcrumbCount);
        OutputDebugStringA(buf);
    }

    if (pf.PageFaultVA)
    {
        sprintf_s(buf, "PageFaultVA=0x%llX\n", (unsigned long long)pf.PageFaultVA);
        OutputDebugStringA(buf);
    }
    OutputDebugStringA("==== DRED End ====\n");
}
