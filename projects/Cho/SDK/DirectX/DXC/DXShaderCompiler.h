#pragma once
#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class DXShaderCompiler
{
public:
	DXShaderCompiler()
	{
		
	}
	~DXShaderCompiler()
	{
	}

	void Initialize(ID3D12Device8* device);
	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);
	ID3D12ShaderReflection* ReflectShader(IDxcBlob* shaderBlob);
private:
	ComPtr<IDxcUtils> m_pUtils;
	ComPtr<IDxcCompiler3> m_pCompiler;
	//ComPtr<IDxcLibrary> m_pLibrary;
	ComPtr<IDxcIncludeHandler> m_pIncludeHandler;
};

