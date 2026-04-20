#include "pathpicker.h"
#include "theme.h"
#include <shobjidl.h>
#include <string>
#include <imgui.h>
#include <cstring>

static HWND g_ownerHwnd = nullptr;

void SetPickerOwner(HWND hwnd) { g_ownerHwnd = hwnd; }

static std::string ToUTF8(PWSTR w)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    return s;
}

static std::string ShowDialog(DWORD extraFlags, const wchar_t* filter)
{
    std::string result;
    IFileOpenDialog* d = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&d))))
        return result;

    DWORD opts; d->GetOptions(&opts);
    d->SetOptions(opts | extraFlags | FOS_FORCEFILESYSTEM);

    if (filter)
    {
        COMDLG_FILTERSPEC specs[8]; UINT count = 0;
        const wchar_t* p = filter;
        while (*p && count < 8)
        {
            specs[count].pszName = p; p += wcslen(p) + 1;
            specs[count].pszSpec = p; p += wcslen(p) + 1;
            count++;
        }
        d->SetFileTypes(count, specs);
    }

    if (SUCCEEDED(d->Show(g_ownerHwnd)))
    {
        IShellItem* item = nullptr;
        if (SUCCEEDED(d->GetResult(&item)))
        {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
            {
                result = ToUTF8(path);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    d->Release();
    return result;
}

std::string PickFolder() { return ShowDialog(FOS_PICKFOLDERS, nullptr); }
std::string PickFile(const wchar_t* filter) { return ShowDialog(0, filter); }

bool PathInput(const char* label, char* buf, size_t bufSize, PathMode mode, const wchar_t* filter)
{
    ImGui::PushID(label);

    ImGui::TextColored(Theme::TextSecondary, "%s", label);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 44.0f);
    ImGui::InputText("##path", buf, bufSize);

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, Theme::BrowseButton);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BrowseButtonHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::BrowseButtonActive);

    bool dirty = false;

    if (ImGui::Button("...", Theme::ButtonSmall))
    {
        dirty = true;

        std::string picked;
        if (mode == PathMode::Folder)
            picked = PickFolder();
        else
            picked = PickFile(filter);

        if (!picked.empty())
            strncpy_s(buf, bufSize, picked.c_str(), _TRUNCATE);
    }

    ImGui::PopStyleColor(3);
    ImGui::PopID();

    return dirty;
}