#pragma once
// === DirectX ===
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
// === C++ Standard Library ===
#include <filesystem>
#include <string>
// === Windows Runtime Library ===
#include <wrl.h>

namespace Theatria::Graphics
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::wstring ShaderProfileToWString(D3D_SHADER_MODEL model);

    /// @brief シェーダーコンパイル設定構造体
    struct ShaderCompileDesc final
    {
        std::wstring filePath;
        std::wstring entryPoint;            // 今は L"main"
        std::wstring profile;             // L"vs_6_0" とか
        bool debug = true;                // true: -Zi/-Od, false: -O3など
    };

    /// @brief シェーダーコンパイラークラス
    class ShaderCompiler final
    {
    public:
        ShaderCompiler() = default;
        ~ShaderCompiler() = default;
        [[nodiscard]] bool Initialize();
        ID3D12ShaderReflection* ReflectShader(IDxcBlob* shaderBlob);

        /// @brief キャッシュ込み
        ComPtr<IDxcBlob> GetOrCompileShader(const ShaderCompileDesc& desc);

        /// @brief DXGIFormatの取得
        DXGI_FORMAT GetDXGIFormat(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE componentMask);
    private:
        std::string SerializeShaderKey(const ShaderCompileDesc& desc,
            std::filesystem::file_time_type lastWrite);
        uint64_t HashShaderDesc(const ShaderCompileDesc& desc, std::filesystem::file_time_type lastWrite);

        /// @brief 生コンパイル
        ComPtr<IDxcBlob> CompileShaderRaw(const ShaderCompileDesc& desc);
        ComPtr<IDxcBlob> LoadBlobFromFile(const std::filesystem::path& path);
        void SaveBlobToFile(const std::filesystem::path& path, IDxcBlob* blob);

        ComPtr<IDxcUtils> m_pUtils = nullptr;
        ComPtr<IDxcCompiler3> m_pCompiler = nullptr;
        ComPtr<IDxcIncludeHandler> m_pIncludeHandler = nullptr;
    };
};

