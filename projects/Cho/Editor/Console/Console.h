#pragma once
#include "Editor/BaseEditor/BaseEditor.h"

#include <deque>
#include <string>
#include <mutex>
#include <cstdarg>
#include <cstdint>
#include <array>
#include "Core/Utility/Color.h"   // あなたの Color 型

class Console : public BaseEditor
{
public:
    enum class LogColor
    {
        White = 0,
        Red,
        Green,
        Blue,
        Yellow,

        _Count
    };

    Console(EditorManager* editorManager) : BaseEditor(editorManager) {}
    ~Console() {}

    void Initialize() override;
    void Update() override;
    void Window() override;

    // 既定色（白）で追加
    void Add(const char* fmt, ...);

    // 色を指定（32bit RGBA: A<<24 | B<<16 | G<<8 | R）
    void AddColored(uint32_t rgba32, const char* fmt, ...);

    // 色を指定（Color）
    void AddColored(const Color& color, const char* fmt, ...);

    void AddColored(LogColor cc, const char* fmt, ...);

    void Clear();

    void SetAutoScroll(bool v) { auto_scroll_ = v; }
    bool GetAutoScroll() const { return auto_scroll_; }

    // 全文コピー（クリップボードへ）
    void CopyAllToClipboard();

private:
    struct ConsoleLine
    {
        uint32_t rgba32;   // ImGui に渡すときは (ImU32) にキャスト
        std::string text;
    };

    // Color -> RGBA32（A<<24 | B<<16 | G<<8 | R）※実装は cpp 側
    static uint32_t PackRGBA32(const Color& c);

    void AddV(uint32_t rgba32, const char* fmt, va_list args);

private:
    std::deque<ConsoleLine> lines_;
    size_t max_lines_{ 5000 };
    bool auto_scroll_{ true };
    bool want_scroll_to_end_{ false };
    char input_buf_[512]{};   // 下部の入力欄
    std::string scratch_;     // クリップボード用
    std::mutex mtx_;
    std::array<uint32_t, static_cast<size_t>(LogColor::_Count)> m_Palette;
};
