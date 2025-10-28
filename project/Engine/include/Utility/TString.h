#pragma once
#include <string>
#include <Windows.h>

namespace Theatria::Utility
{
    /// @brief UTF-8文字列をUTF-16文字列に変換します。
    /// @param utf8Str UTF-8文字列
    /// @return UTF-16文字列
    std::wstring ToUTF16(const std::string& utf8Str);
    /// @brief UTF-16文字列をUTF-8文字列に変換します。
    /// @param utf16Str UTF-16文字列
    /// @return UTF-8文字列
    std::string ToUTF8(const std::wstring& utf16Str);

    /// @brief TStringクラス
    class TString final
    {
    public:
        
    };
};

