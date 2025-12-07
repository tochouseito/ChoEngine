#include "pch.h"
#include "include/Graphics/ShaderCompiler.h"
#include "include/Core/LogAssert.h"
#include "config/engineConfig.h"
#include "include/Utility/TString.h"

using namespace Theatria::Graphics;

bool Theatria::Graphics::ShaderCompiler::Initialize()
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

ComPtr<IDxcBlob> Theatria::Graphics::ShaderCompiler::GetOrCompileShader(const ShaderCompileDesc& desc)
{
    // 1. キー生成
    //auto lastWrite = std::filesystem::last_write_time(desc.filePath);
    //uint64_t key = HashShaderDesc(desc, lastWrite); // 実装は好きに

    //std::wstringstream ss;
    //ss << std::hex << key;
    //std::filesystem::path cachePath = std::filesystem::path(Config::FilePath::ShaderCacheDirectory) / (ss.str() + L".dxil");

    //// 2. キャッシュファイルがあれば読み込んで終わり
    //if (std::filesystem::exists(cachePath))
    //{
    //    return LoadBlobFromFile(cachePath);
    //}

    // 3. 無ければコンパイルして保存
    ComPtr<IDxcBlob> blob = CompileShaderRaw(desc);
    //SaveBlobToFile(cachePath, blob.Get());
    return blob;
}

DXGI_FORMAT Theatria::Graphics::ShaderCompiler::GetDXGIFormat(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE componentMask)
{
    // 成分数
    UINT componentCount = 0;
    if (componentMask & 0x1) ++componentCount; // x
    if (componentMask & 0x2) ++componentCount; // y
    if (componentMask & 0x4) ++componentCount; // z
    if (componentMask & 0x8) ++componentCount; // w

    if (componentCount == 0)
        return DXGI_FORMAT_UNKNOWN;

    switch (componentType)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        switch (componentCount)
        {
        case 1: return DXGI_FORMAT_R32_UINT;
        case 2: return DXGI_FORMAT_R32G32_UINT;
        case 3: return DXGI_FORMAT_R32G32B32_UINT;
        case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
        }
        break;

    case D3D_REGISTER_COMPONENT_SINT32:
        switch (componentCount)
        {
        case 1: return DXGI_FORMAT_R32_SINT;
        case 2: return DXGI_FORMAT_R32G32_SINT;
        case 3: return DXGI_FORMAT_R32G32B32_SINT;
        case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
        }
        break;

    case D3D_REGISTER_COMPONENT_FLOAT32:
        switch (componentCount)
        {
        case 1: return DXGI_FORMAT_R32_FLOAT;
        case 2: return DXGI_FORMAT_R32G32_FLOAT;
        case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        break;
    }

    return DXGI_FORMAT_UNKNOWN;
}

std::string Theatria::Graphics::ShaderCompiler::SerializeShaderKey(const ShaderCompileDesc& desc, std::filesystem::file_time_type lastWrite)
{
    std::ostringstream oss;
    oss << Utility::ToUTF8(desc.filePath)
        << "|" << Utility::ToUTF8(desc.entryPoint)
        << "|" << Utility::ToUTF8(desc.profile)
        << "|" << (desc.debug ? "D" : "R")
        << "|" << lastWrite.time_since_epoch().count();

    // defines があればそこも追加
    return oss.str();
}

uint64_t Theatria::Graphics::ShaderCompiler::HashShaderDesc(const ShaderCompileDesc& desc, std::filesystem::file_time_type lastWrite)
{
    std::string keyStr = SerializeShaderKey(desc, lastWrite);
    uint64_t h = 1469598103934665603ull; // FNV-1a 64bit offset basis
    for (unsigned char c : keyStr)
    {
        h ^= c;
        h *= 1099511628211ull;
    }
    return h;
}

ComPtr<IDxcBlob> Theatria::Graphics::ShaderCompiler::CompileShaderRaw(const ShaderCompileDesc& desc)
{
    HRESULT hr = {};
    ComPtr<IDxcBlobEncoding> pSource = nullptr;
    hr = m_pUtils.Get()->LoadFile(desc.filePath.c_str(), nullptr, &pSource);
    Core::LogAssert::Check(hr, "ShaderCompiler", "DXC LoadFile Failed!!");
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = pSource->GetBufferPointer();
    sourceBuffer.Size = pSource->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;
    LPCWSTR arguments[] = {
        desc.filePath.c_str(),              //コンパイル対象のhlslファイル名
        L"-E",desc.entryPoint.c_str(),                      // エントリーポイントの指定。基本的にmain以外にはしない
        L"-T",desc.profile.c_str(),         // ShaderProfileの設定
        L"-Zi",L"-Qembed_debug",            // デバッグ用の情報を埋め込む
        L"-Od",                             // 最適化を外しておく
        L"-Zpr",                            // メモリレイアウトは行優先
    };

    std::vector<LPCWCH> args;
    args.push_back(desc.filePath.c_str());// コンパイル対象のhlslファイル名
    args.push_back(L"-E");
    args.push_back(desc.entryPoint.c_str());// エントリーポイントの指定。基本的にmain以外にはしない
    args.push_back(L"-T");
    args.push_back(desc.profile.c_str());// ShaderProfileの設定
    args.push_back(L"-Zpr");// メモリレイアウトは行優先
    if (desc.debug)
    {
        args.push_back(L"-Zi");
        args.push_back(L"-Qembed_debug");// デバッグ情報埋め込み
        args.push_back(L"-Od");// デバッグビルドなら最適化外す
    }
    else
    {
        args.push_back(L"-O3");// リリースビルドなら最適化最大
    }

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
        Core::LogAssert::LogRuntime(std::source_location::current(), Core::LogAssert::SinkKind::Console,
            Core::LogAssert::LogLevel::Error,
            "ShaderCompiler", pErrors->GetStringPointer());
        Core::LogAssert::Check(false, "ShaderCompiler", "DXC Compile Error!!");
    }
    IDxcBlob* pShader = nullptr;
    hr = pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pErrorsUtf16);
    return pShader;
}

ComPtr<IDxcBlob> Theatria::Graphics::ShaderCompiler::LoadBlobFromFile(const std::filesystem::path& path)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs)
    {
        return nullptr;
    }
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!ifs.read(reinterpret_cast<char*>(data.data()), size))
    {
        return nullptr;
    }

    ComPtr<IDxcBlob> blob;
    // バイナリなので encoding は正直どうでもいい
    m_pUtils->CreateBlob(data.data(), static_cast<UINT32>(size), DXC_CP_ACP,
        reinterpret_cast<IDxcBlobEncoding**>(blob.GetAddressOf()));
    return blob;
}

void Theatria::Graphics::ShaderCompiler::SaveBlobToFile(const std::filesystem::path& path, IDxcBlob* blob)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
    {
        Core::LogAssert::Verify(false, "ShaderCompiler", "Failed to open shader cache file for writing!!");
        return;
    }
    ofs.write(static_cast<const char*>(blob->GetBufferPointer()),
        blob->GetBufferSize());
}

std::wstring Theatria::Graphics::ShaderProfileToWString(D3D_SHADER_MODEL model)
{
    switch (model)
    {
    case D3D_SHADER_MODEL_NONE:
        return L"Unknown Model";
        break;
    case D3D_SHADER_MODEL_5_1:
        return L"5_1";
        break;
    case D3D_SHADER_MODEL_6_0:
        return L"6_0";
        break;
    case D3D_SHADER_MODEL_6_1:
        return L"6_1";
        break;
    case D3D_SHADER_MODEL_6_2:
        return L"6_2";
        break;
    case D3D_SHADER_MODEL_6_3:
        return L"6_3";
        break;
    case D3D_SHADER_MODEL_6_4:
        return L"6_4";
        break;
    case D3D_SHADER_MODEL_6_5:
        return L"6_5";
        break;
    case D3D_SHADER_MODEL_6_6:
        return L"6_6";
        break;
    case D3D_SHADER_MODEL_6_7:
        return L"6_7";
        break;
    case D3D_SHADER_MODEL_6_8:
        return L"6_8";
        break;
    case D3D_SHADER_MODEL_6_9:
        return L"6_9";
        break;
    default:
        return L"Unknown Model";
        break;
    }
}
