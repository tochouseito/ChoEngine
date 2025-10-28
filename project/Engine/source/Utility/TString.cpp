#include "pch.h"
#include "include/Utility/TString.h"

std::wstring Theatria::Utility::ToUTF16(const std::string& utf8Str)
{
    if (utf8Str.empty()) return {};
    int size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8Str.c_str(), static_cast<int>(utf8Str.size()),
        nullptr, 0);
    if (size <= 0) return {};
    std::wstring ws(size, L'\0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8Str.c_str(), static_cast<int>(utf8Str.size()),
        ws.data(), size);
    return ws;
}

std::string Theatria::Utility::ToUTF8(const std::wstring& utf16Str)
{
    if (utf16Str.empty()) return {};
    // 無効文字検出を有効化
    int size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16Str.c_str(), static_cast<int>(utf16Str.size()),
        nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string utf8(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16Str.c_str(), static_cast<int>(utf16Str.size()),
        utf8.data(), size, nullptr, nullptr);
    return utf8;
}
