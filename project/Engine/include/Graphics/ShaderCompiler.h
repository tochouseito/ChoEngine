#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <filesystem>
#include <string>
#include <wrl.h>

namespace Theatria::Graphics
{
    class ShaderCompiler final
    {
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    public:
        ShaderCompiler() = default;
        ~ShaderCompiler() = default;
        [[nodiscard]] bool Initialize(ID3D12Device* device);
        IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);
        ID3D12ShaderReflection* ReflectShader(IDxcBlob* shaderBlob);
    private:
        ComPtr<IDxcUtils> m_pUtils = nullptr;
        ComPtr<IDxcCompiler3> m_pCompiler = nullptr;
        ComPtr<IDxcIncludeHandler> m_pIncludeHandler = nullptr;
    };
};

