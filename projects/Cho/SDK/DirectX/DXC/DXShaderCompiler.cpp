#include "pch.h"
#include "DXShaderCompiler.h"
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;

void DXShaderCompiler::Initialize(ID3D12Device8* device)
{
	device;
	HRESULT hr = {};
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pUtils));
	Log::Write(LogLevel::Assert, "DXC Utils created", hr);
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler));
	Log::Write(LogLevel::Assert, "DXC Compiler created", hr);
	hr = m_pUtils->CreateDefaultIncludeHandler(&m_pIncludeHandler);
	Log::Write(LogLevel::Assert, "DXC Include Handler created", hr);
}

IDxcBlob* DXShaderCompiler::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
	HRESULT hr = {};
	Log::Write(LogLevel::Info, ConvertString(std::format(L"Begin CompilerShader,path:{},profile:{}\n", filePath, profile)).c_str());
	ComPtr<IDxcBlobEncoding> pSource = nullptr;
	hr = m_pUtils.Get()->LoadFile(filePath.c_str(), nullptr, &pSource);
	Log::Write(LogLevel::Assert, "DXC LoadFile", hr);
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
	Log::Write(LogLevel::Assert, "DXC Compile", hr);
	ComPtr<IDxcBlobUtf8> pErrors = nullptr;
	ComPtr<IDxcBlobUtf16> pErrorsUtf16;
	hr = pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), &pErrorsUtf16);
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		Log::Write(LogLevel::Error, pErrors->GetStringPointer());
		Log::Write(LogLevel::Assert, "DXC Compile Error");
	}
	IDxcBlob* pShader = nullptr;
	hr = pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pErrorsUtf16);
	Log::Write(LogLevel::Assert, "DXC Compile Succeeded", hr);
	Log::Write(LogLevel::Info, ConvertString(std::format(L"End CompilerShader,path:{},profile:{}\n", filePath, profile)).c_str());
	return pShader;
}

ID3D12ShaderReflection* DXShaderCompiler::ReflectShader(IDxcBlob* shaderBlob)
{
	HRESULT hr = {};
	ComPtr<IDxcContainerReflection> pReflection = nullptr;
	hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&pReflection));
	Log::Write(LogLevel::Assert, "DXC Container Reflection created", hr);
	hr = pReflection->Load(shaderBlob);
	Log::Write(LogLevel::Assert, "DXC Load shader blob", hr);
	UINT32 partIndex = 0;
	hr = pReflection->FindFirstPartKind(DXC_PART_DXIL, &partIndex);
	Log::Write(LogLevel::Assert, "DXC Find first part kind", hr);
	ID3D12ShaderReflection* pShaderReflection = nullptr;
	hr = pReflection->GetPartReflection(partIndex, IID_PPV_ARGS(&pShaderReflection));
	Log::Write(LogLevel::Assert, "DXC Get part reflection", hr);
	return pShaderReflection;
}
