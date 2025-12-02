#pragma once
#include <string>
#include <regex>
#include <unordered_map>
#include <set>

namespace theatria
{
    // ユーティリティ：`container` に `key` が含まれているかチェック（ADLで見つかる `find` を使う）
    template<typename Container, typename Key>
    bool ContainsKey(const Container& container, const Key& key)
    {
        return container.find(key) != container.end();
    }

    template<typename Container>
    inline std::wstring GenerateUniqueName(
        const std::wstring& baseName,
        const Container& nameList)
    {
        std::wstring base;
        int counter = 1;

        // 修正: 空白なし "Name(1)" にも対応
        std::wregex numberRegex(LR"(^(.+?)\((\d+)\)$)");
        std::wsmatch match;

        if (std::regex_match(baseName, match, numberRegex))
        {
            base = match[1];
            counter = std::stoi(match[2]) + 1;
        }
        else
        {
            base = baseName;
            if (!ContainsKey(nameList, baseName))
                return baseName;
        }

        std::wstring newName;
        do
        {
            newName = base + L"(" + std::to_wstring(counter++) + L")";
        } while (ContainsKey(nameList, newName));

        return newName;
    }
}