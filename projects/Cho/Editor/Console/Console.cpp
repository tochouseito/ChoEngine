#include "pch.h"
#include "Console.h"
#include "Editor/EditorManager/EditorManager.h"

/*========================
  Console lifecycle
========================*/
void Console::Initialize()
{
    // カラーパレット初期化
    m_Palette[static_cast<size_t>(LogColor::White)] = Color::RGBAu8(255, 255, 255);
    m_Palette[static_cast<size_t>(LogColor::Red)] = Color::RGBAu8(255, 100, 100);
    m_Palette[static_cast<size_t>(LogColor::Green)] = Color::RGBAu8(100, 255, 100);
    m_Palette[static_cast<size_t>(LogColor::Blue)] = Color::RGBAu8(100, 100, 255);
    m_Palette[static_cast<size_t>(LogColor::Yellow)] = Color::RGBAu8(255, 255, 100);
}

void Console::Update()
{
    Window();
}

/*========================
  Window (UI)
========================*/
void Console::Window()
{
    // ウィンドウ背景を黒に
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255));
    bool opened = ImGui::Begin("Console");
    ImGui::PopStyleColor(); // WindowBg

    if (!opened) { ImGui::End(); return; }

    // 上：スクロール可能ログ領域（Child 背景も黒）
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 255));

    const float input_zone_h = ImGui::GetFrameHeightWithSpacing() * 1.6f;
    ImGui::BeginChild("LogRegion", ImVec2(0, -input_zone_h), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    // ログ描画（大量行に強い）
    {
        std::lock_guard<std::mutex> lk(mtx_);

        const float line_h = ImGui::GetTextLineHeightWithSpacing();        // 1行の高さ（間隔込み）
        const float content_h = line_h * static_cast<float>(lines_.size());   // 全行の高さ（単一行前提）
        const float region_h = ImGui::GetContentRegionAvail().y;             // 子ウィンドウの表示領域
        const float pad = region_h - content_h;
        if (pad > 0.0f)
        {
            // 余白ダミーで押し下げる（描画原点は上なので、先頭に入れると内容が下に寄る）
            ImGui::Dummy(ImVec2(0.0f, pad));
        }

        ImGuiListClipper clip;
        clip.Begin((int)lines_.size());
        while (clip.Step())
        {
            for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i)
            {
                const ConsoleLine& L = lines_[i];
                ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)L.rgba32);
                ImGui::TextUnformatted(L.text.c_str());
                ImGui::PopStyleColor();
            }
        }
        if (auto_scroll_)
        {
            if (want_scroll_to_end_ || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            want_scroll_to_end_ = false;
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg

    // 下：入力バー（幅いっぱい）
    ImGui::PushItemWidth(-1.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 20, 20, 255));

    if (ImGui::InputText("##console_input", input_buf_, IM_ARRAYSIZE(input_buf_),
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (input_buf_[0] != '\0')
        {
            Add("%s", input_buf_);     // 既定色（白）で上に流れる
            input_buf_[0] = '\0';
        }
    }

    // 右クリックメニュー
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Copy all")) CopyAllToClipboard();
        if (ImGui::MenuItem("Clear"))    Clear();
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    ImGui::End();
}

/*========================
  Public logging APIs
========================*/
void Console::Add(const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    AddV((uint32_t)IM_COL32_WHITE, fmt, args);
    va_end(args);
}

void Console::AddColored(uint32_t rgba32, const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    AddV(rgba32, fmt, args);
    va_end(args);
}

void Console::AddColored(const Color& color, const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    AddV(PackRGBA32(color), fmt, args);
    va_end(args);
}

void Console::AddColored(LogColor cc, const char* fmt, ...)
{
    const uint32_t col = m_Palette[static_cast<size_t>(cc)];
    va_list args; va_start(args, fmt);
    AddV(col, fmt, args);
    va_end(args);
}

void Console::Clear()
{
    std::lock_guard<std::mutex> lk(mtx_);
    lines_.clear();
    want_scroll_to_end_ = true;
}

void Console::CopyAllToClipboard()
{
    std::lock_guard<std::mutex> lk(mtx_);
    scratch_.clear();
    scratch_.reserve(lines_.size() * 32);
    for (auto& L : lines_)
    {
        scratch_ += L.text;
        scratch_.push_back('\n');
    }
    ImGui::SetClipboardText(scratch_.c_str());
}

/*========================
  Private helpers
========================*/
void Console::AddV(uint32_t rgba32, const char* fmt, va_list args)
{
    char buf[4096];
#if defined(_WIN32)
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
#else
    vsnprintf(buf, sizeof(buf), fmt, args);
#endif

    std::lock_guard<std::mutex> lk(mtx_);
    if (lines_.size() >= max_lines_) lines_.pop_front();
    lines_.push_back(ConsoleLine{ rgba32, buf });
    want_scroll_to_end_ = true;
}

// ImGuiの IM_COL32(R,G,B,A) と同じ packing（A<<24 | B<<16 | G<<8 | R）
uint32_t Console::PackRGBA32(const Color& c)
{
    auto clamp01 = [](float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
    const uint32_t r = (uint32_t)(clamp01(c.r) * 255.0f + 0.5f);
    const uint32_t g = (uint32_t)(clamp01(c.g) * 255.0f + 0.5f);
    const uint32_t b = (uint32_t)(clamp01(c.b) * 255.0f + 0.5f);
    const uint32_t a = (uint32_t)(clamp01(c.a) * 255.0f + 0.5f);
    return (a << 24) | (b << 16) | (g << 8) | (r);
}
