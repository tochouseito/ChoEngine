#include "pch.h"
#include "include/Graphics/ShaderCompiler.h"
#include "include/Core/LogAssert.h"

bool Theatria::Graphics::ShaderCompiler::Initialize(ID3D12Device* device)
{
    if (!Core::LogAssert::Verify(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pUtils)),
        "ShaderCompiler", "DXC Utils Create Failed!!"))
    {
        return false;
    }
    if (!Core::LogAssert::Verify(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler)),
        "ShaderCompiler", "DXC Compiler Create Failed!!"))
    {
        return false;
    }
    if (!Core::LogAssert::Verify(m_pUtils->CreateDefaultIncludeHandler(&m_pIncludeHandler),
        "ShaderCompiler", "DXC Include Handler Create Failed!!"))
    {
        return false;
    }
    return true;
}

IDxcBlob* Theatria::Graphics::ShaderCompiler::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
    HRESULT hr = {};
    ComPtr<IDxcBlobEncoding> pSource = nullptr;
    hr = m_pUtils.Get()->LoadFile(filePath.c_str(), nullptr, &pSource);
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC LoadFile Failed!!");
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = pSource->GetBufferPointer();
    sourceBuffer.Size = pSource->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;
    LPCWSTR arguments[] = {
        filePath.c_str(),       //コンパイル対象のhlslファイル名
        L"-E",L"main",          // エントリーポイントの指定。基本的にmain以外にはしない
        L"-T",profile,          // ShaderProfileの設定
        L"-Zi",L"-Qembed_debug",// デバッグ用の情報を埋め込む
        L"-Od",                 // 最適化を外しておく
        L"-Zpr",                // メモリレイアウトは行優先
    };

    ComPtr<IDxcResult> pResult = nullptr;
    hr = m_pCompiler.Get()->Compile(
        &sourceBuffer,			// 読み込んだファイル
        arguments,				// コンパイルオプション
        _countof(arguments),	// コンパイル結果
        m_pIncludeHandler.Get(),// includeが含まれた諸々
        IID_PPV_ARGS(&pResult)	// コンパイル結果
    );
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC Compile Failed!!");
    ComPtr<IDxcBlobUtf8> pErrors = nullptr;
    ComPtr<IDxcBlobUtf16> pErrorsUtf16;
    hr = pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), &pErrorsUtf16);
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
        Core::LogAssert::Log(std::source_location::current(), Core::LogAssert::SinkKind::Console,
            Core::LogAssert::LogLevel::Error,
            "ShaderCompiler", pErrors->GetStringPointer());
        Core::LogAssert::Check(false, "ShaderCompiler", "DXC Compile Error!!");
    }
    IDxcBlob* pShader = nullptr;
    hr = pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pErrorsUtf16);
    return pShader;
}

ID3D12ShaderReflection* Theatria::Graphics::ShaderCompiler::ReflectShader(IDxcBlob* shaderBlob)
{
    HRESULT hr = {};
    ComPtr<IDxcContainerReflection> pContainerReflection = nullptr;
    hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&pContainerReflection));
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC Container Reflection Create Failed!!");
    hr = pContainerReflection->Load(shaderBlob);
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC Container Reflection Load Failed!!");
    UINT32 partIndex = 0;
    hr = pContainerReflection->FindFirstPartKind(DXC_PART_DXIL, &partIndex);
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC FindFirstPartKind Failed!!");
    ID3D12ShaderReflection* pShaderReflection = nullptr;
    hr = pContainerReflection->GetPartReflection(partIndex, IID_PPV_ARGS(&pShaderReflection));
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC GetPartReflection Failed!!");
    return pShaderReflection;
}
