#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <knownfolders.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT WM_STATUS_UPDATE = WM_APP + 2;
constexpr UINT ID_TRAY = 100;
constexpr UINT ID_ENABLE = 200;
constexpr UINT ID_OPEN_SETTINGS = 201;
constexpr UINT ID_OPEN_GITHUB = 202;
constexpr UINT ID_CHOOSE_CFG = 203;
constexpr UINT ID_LANG_AUTO = 204;
constexpr UINT ID_LANG_EN = 205;
constexpr UINT ID_LANG_ZH = 206;
constexpr UINT ID_QUIT = 207;
constexpr UINT ID_STATUS = 300;
constexpr UINT ID_GUI_ENABLE = 301;
constexpr UINT ID_HIDE = 302;
constexpr UINT ID_TEST_SOUND = 303;
constexpr UINT ID_GSI_STATUS = 304;
constexpr UINT ID_LAST_ACTION = 305;

constexpr wchar_t AppName[] = L"CS2RoundAlert";
constexpr wchar_t RepositoryUrl[] = L"https://github.com/BannerLord54/CS2RoundAlert";
constexpr wchar_t ConfigFileName[] = L"gamestate_integration_cs2roundalert.cfg";
constexpr wchar_t Cs2FolderName[] = L"Counter-Strike Global Offensive";

struct Settings
{
    int port = 3000;
    bool enabled = true;
    bool alertOnlyWhenNotFocused = true;
    bool useSystemSound = true;
    std::wstring customWavPath;
    std::wstring cs2CfgFolderPath;
    bool firstRunNotificationShown = false;
    std::wstring language = L"auto";
};

std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty())
    {
        return {};
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length);
    return result;
}

std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty())
    {
        return {};
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring Lower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return value;
}

void AddUnique(std::vector<std::wstring>& values, const std::wstring& value)
{
    if (value.empty())
    {
        return;
    }

    fs::path path(value);
    std::wstring normalized = path.make_preferred().wstring();
    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L'/'))
    {
        normalized.pop_back();
    }

    const std::wstring lowered = Lower(normalized);
    const bool exists = std::any_of(values.begin(), values.end(), [&](const std::wstring& existing) {
        return Lower(existing) == lowered;
    });

    if (!exists)
    {
        values.push_back(normalized);
    }
}

std::string ReadText(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return {};
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

void WriteText(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
}

std::wstring AppDataDirectory()
{
    PWSTR rawPath = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &rawPath)))
    {
        result = rawPath;
        CoTaskMemFree(rawPath);
    }

    return (fs::path(result) / AppName).wstring();
}

std::wstring ReadRegistryString(HKEY root, const std::wstring& subkey, const std::wstring& valueName)
{
    const REGSAM views[] = { KEY_READ, KEY_READ | KEY_WOW64_32KEY, KEY_READ | KEY_WOW64_64KEY };

    for (REGSAM view : views)
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(root, subkey.c_str(), 0, view, &key) != ERROR_SUCCESS)
        {
            continue;
        }

        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
            (type != REG_SZ && type != REG_EXPAND_SZ) || size == 0)
        {
            RegCloseKey(key);
            continue;
        }

        std::wstring value(size / sizeof(wchar_t), L'\0');
        const LONG read = RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(value.data()), &size);
        RegCloseKey(key);

        if (read != ERROR_SUCCESS)
        {
            continue;
        }

        while (!value.empty() && value.back() == L'\0')
        {
            value.pop_back();
        }

        std::replace(value.begin(), value.end(), L'/', L'\\');

        if (type == REG_EXPAND_SZ)
        {
            wchar_t expanded[MAX_PATH]{};
            ExpandEnvironmentStringsW(value.c_str(), expanded, MAX_PATH);
            return expanded;
        }

        return value;
    }

    return {};
}

std::wstring SettingsPath()
{
    return (fs::path(AppDataDirectory()) / L"settings.json").wstring();
}

std::wstring LogPath()
{
    return (fs::path(AppDataDirectory()) / L"CS2RoundAlert.log").wstring();
}

void AppendLog(const std::wstring& message)
{
    fs::create_directories(AppDataDirectory());

    SYSTEMTIME time{};
    GetLocalTime(&time);

    wchar_t prefix[64]{};
    swprintf_s(
        prefix,
        L"%04u-%02u-%02u %02u:%02u:%02u ",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond);

    std::ofstream file(fs::path(LogPath()), std::ios::binary | std::ios::app);
    file << WideToUtf8(std::wstring(prefix) + message + L"\n");
}

std::string EscapeJson(const std::wstring& value)
{
    std::string utf8 = WideToUtf8(value);
    std::string result;
    result.reserve(utf8.size());

    for (char c : utf8)
    {
        if (c == '\\' || c == '"')
        {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    return result;
}

std::string UnescapeJsonString(std::string value)
{
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\\' && i + 1 < value.size())
        {
            result.push_back(value[++i]);
            continue;
        }

        result.push_back(value[i]);
    }

    return result;
}

bool ReadBool(const std::string& json, const std::string& name, bool fallback)
{
    const std::regex pattern("\"" + name + "\"\\s*:\\s*(true|false)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(json, match, pattern))
    {
        return match[1].str() == "true" || match[1].str() == "TRUE";
    }

    return fallback;
}

int ReadInt(const std::string& json, const std::string& name, int fallback)
{
    const std::regex pattern("\"" + name + "\"\\s*:\\s*(\\d+)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(json, match, pattern))
    {
        const int value = std::stoi(match[1].str());
        return value > 0 && value <= 65535 ? value : fallback;
    }

    return fallback;
}

std::wstring ReadString(const std::string& json, const std::string& name, const std::wstring& fallback)
{
    const std::regex pattern("\"" + name + "\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(json, match, pattern))
    {
        return Utf8ToWide(UnescapeJsonString(match[1].str()));
    }

    return fallback;
}

Settings LoadSettings()
{
    Settings settings;
    const std::string json = ReadText(SettingsPath());
    if (json.empty())
    {
        return settings;
    }

    settings.port = ReadInt(json, "port", settings.port);
    settings.enabled = ReadBool(json, "enabled", settings.enabled);
    settings.alertOnlyWhenNotFocused = ReadBool(json, "alertOnlyWhenNotFocused", settings.alertOnlyWhenNotFocused);
    settings.useSystemSound = ReadBool(json, "useSystemSound", settings.useSystemSound);
    settings.customWavPath = ReadString(json, "customWavPath", settings.customWavPath);
    settings.cs2CfgFolderPath = ReadString(json, "cs2CfgFolderPath", settings.cs2CfgFolderPath);
    settings.firstRunNotificationShown = ReadBool(json, "firstRunNotificationShown", settings.firstRunNotificationShown);
    settings.language = ReadString(json, "language", settings.language);

    if (settings.language != L"auto" && settings.language != L"en" && settings.language != L"zh-CN")
    {
        settings.language = L"auto";
    }

    return settings;
}

void SaveSettings(const Settings& settings)
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"port\": " << settings.port << ",\n";
    json << "  \"enabled\": " << (settings.enabled ? "true" : "false") << ",\n";
    json << "  \"alertOnlyWhenNotFocused\": " << (settings.alertOnlyWhenNotFocused ? "true" : "false") << ",\n";
    json << "  \"useSystemSound\": " << (settings.useSystemSound ? "true" : "false") << ",\n";
    json << "  \"customWavPath\": \"" << EscapeJson(settings.customWavPath) << "\",\n";
    json << "  \"cs2CfgFolderPath\": \"" << EscapeJson(settings.cs2CfgFolderPath) << "\",\n";
    json << "  \"firstRunNotificationShown\": " << (settings.firstRunNotificationShown ? "true" : "false") << ",\n";
    json << "  \"language\": \"" << EscapeJson(settings.language) << "\"\n";
    json << "}\n";

    WriteText(SettingsPath(), json.str());
}

bool UseChinese(const Settings& settings)
{
    if (settings.language == L"zh-CN")
    {
        return true;
    }

    if (settings.language == L"en")
    {
        return false;
    }

    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE;
}

std::wstring Text(const Settings& settings, const std::wstring& key)
{
    const bool zh = UseChinese(settings);

    if (key == L"EnableAlerts") return zh ? L"开启提示音" : L"Enable alerts";
    if (key == L"OpenSettingsFolder") return zh ? L"打开设置文件夹" : L"Open settings folder";
    if (key == L"OpenGitHubRepo") return zh ? L"打开 GitHub 仓库" : L"Open GitHub repo";
    if (key == L"ChooseCfgFolder") return zh ? L"选择 CS2 cfg 文件夹" : L"Choose CS2 cfg folder";
    if (key == L"StatusEnabled") return zh ? L"提醒：开启" : L"Alerts: On";
    if (key == L"StatusDisabled") return zh ? L"提醒：关闭" : L"Alerts: Off";
    if (key == L"HideToTray") return zh ? L"隐藏到托盘" : L"Hide to tray";
    if (key == L"Language") return zh ? L"语言" : L"Language";
    if (key == L"LanguageAuto") return zh ? L"自动（跟随系统）" : L"Auto (system)";
    if (key == L"LanguageEnglish") return L"English";
    if (key == L"LanguageChinese") return L"中文";
    if (key == L"Quit") return zh ? L"退出" : L"Quit";
    if (key == L"SelectCfgDescription") return zh ? L"选择 CS2 cfg 文件夹，或选择 Counter-Strike Global Offensive 文件夹。" : L"Select the CS2 cfg folder or the Counter-Strike Global Offensive folder.";
    if (key == L"InvalidCfgFolder") return zh ? L"这个文件夹不像 CS2 cfg 文件夹。请选择 Counter-Strike Global Offensive\\game\\csgo\\cfg。" : L"That folder does not look like the CS2 cfg folder. Select Counter-Strike Global Offensive\\game\\csgo\\cfg.";
    if (key == L"GsiConfigNotInstalled") return zh ? L"未安装 GSI 配置。请重启程序并选择 CS2 cfg 文件夹。" : L"GSI config was not installed. Restart the app to choose the CS2 cfg folder.";
    if (key == L"GsiConfigInstalled") return zh ? L"GSI 配置已安装到：" : L"GSI config installed at:";
    if (key == L"GsiConfigWriteFailed") return zh ? L"无法写入 GSI 配置：" : L"Could not write the GSI config:";
    if (key == L"ListenFailed") return zh ? L"无法监听本地端口。" : L"Could not listen on the local port.";
    if (key == L"Listening") return zh ? L"\u76d1\u542c\uff1a127.0.0.1:" : L"Listening: 127.0.0.1:";
    if (key == L"NoGsiYet") return zh ? L"GSI\uff1a\u8fd8\u6ca1\u6536\u5230 CS2 \u6570\u636e" : L"GSI: no CS2 data received yet";
    if (key == L"GsiNoRound") return zh ? L"GSI\uff1a\u5df2\u6536\u5230\u6570\u636e\uff0c\u4f46\u6ca1\u6709 round.phase" : L"GSI: received data, no round.phase";
    if (key == L"GsiPhase") return zh ? L"GSI\uff1around.phase = " : L"GSI: round.phase = ";
    if (key == L"WaitingFreezetime") return zh ? L"\u7b49\u5f85\u4e0b\u4e00\u6b21 freezetime" : L"Waiting for next freezetime";
    if (key == L"AlertPlayed") return zh ? L"\u5df2\u64ad\u653e\u63d0\u793a\u97f3" : L"Alert played";
    if (key == L"AlertSkippedDisabled") return zh ? L"\u672a\u64ad\u653e\uff1a\u63d0\u9192\u5df2\u5173\u95ed" : L"Skipped: alerts disabled";
    if (key == L"AlertSkippedForeground") return zh ? L"\u672a\u64ad\u653e\uff1aCS2 \u6b63\u5728\u524d\u53f0" : L"Skipped: CS2 is foreground";
    if (key == L"TestSound") return zh ? L"\u6d4b\u8bd5\u63d0\u793a\u97f3" : L"Test sound";
    if (key == L"TestSoundPlayed") return zh ? L"\u5df2\u64ad\u653e\u6d4b\u8bd5\u63d0\u793a\u97f3" : L"Test sound played";

    return key;
}

std::vector<std::wstring> SteamRootCandidates()
{
    std::vector<std::wstring> roots;
    wchar_t programFilesX86[MAX_PATH]{};
    wchar_t programFiles[MAX_PATH]{};
    GetEnvironmentVariableW(L"ProgramFiles(x86)", programFilesX86, MAX_PATH);
    GetEnvironmentVariableW(L"ProgramFiles", programFiles, MAX_PATH);

    AddUnique(roots, (fs::path(programFilesX86) / L"Steam").wstring());
    AddUnique(roots, (fs::path(programFiles) / L"Steam").wstring());

    const std::vector<std::pair<HKEY, std::wstring>> steamKeys = {
        { HKEY_CURRENT_USER, L"Software\\Valve\\Steam" },
        { HKEY_LOCAL_MACHINE, L"Software\\Valve\\Steam" },
        { HKEY_LOCAL_MACHINE, L"Software\\WOW6432Node\\Valve\\Steam" }
    };

    for (const auto& [root, key] : steamKeys)
    {
        AddUnique(roots, ReadRegistryString(root, key, L"SteamPath"));
        AddUnique(roots, ReadRegistryString(root, key, L"InstallPath"));

        const std::wstring steamExe = ReadRegistryString(root, key, L"SteamExe");
        if (!steamExe.empty())
        {
            AddUnique(roots, fs::path(steamExe).parent_path().wstring());
        }
    }

    const DWORD drives = GetLogicalDrives();
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter)
    {
        if ((drives & (1 << (letter - L'A'))) == 0)
        {
            continue;
        }

        const std::wstring drive = std::wstring(1, letter) + L":\\";
        if (GetDriveTypeW(drive.c_str()) != DRIVE_FIXED)
        {
            continue;
        }

        AddUnique(roots, (fs::path(drive) / L"SteamLibrary").wstring());
        AddUnique(roots, (fs::path(drive) / L"Steam").wstring());
        AddUnique(roots, (fs::path(drive) / L"Games" / L"Steam").wstring());
        AddUnique(roots, (fs::path(drive) / L"Program Files (x86)" / L"Steam").wstring());
        AddUnique(roots, (fs::path(drive) / L"Program Files" / L"Steam").wstring());
    }

    return roots;
}

std::vector<std::wstring> SteamLibraries()
{
    std::vector<std::wstring> libraries;
    const std::regex pattern("\"path\"\\s+\"([^\"]+)\"", std::regex_constants::icase);

    for (const std::wstring& root : SteamRootCandidates())
    {
        if (!fs::exists(root))
        {
            continue;
        }

        AddUnique(libraries, root);

        const fs::path libraryFile = fs::path(root) / L"steamapps" / L"libraryfolders.vdf";
        const std::string content = ReadText(libraryFile);
        if (content.empty())
        {
            continue;
        }

        for (std::sregex_iterator it(content.begin(), content.end(), pattern), end; it != end; ++it)
        {
            std::string raw = (*it)[1].str();
            for (size_t pos = 0; (pos = raw.find("\\\\", pos)) != std::string::npos;)
            {
                raw.replace(pos, 2, "\\");
                ++pos;
            }

            const std::wstring path = Utf8ToWide(raw);
            if (fs::exists(path))
            {
                AddUnique(libraries, path);
            }
        }
    }

    return libraries;
}

std::wstring ResolveCfgDirectory(const std::wstring& selected)
{
    const fs::path base(selected);
    const std::vector<fs::path> candidates = {
        base,
        base / L"game" / L"csgo" / L"cfg",
        base / Cs2FolderName / L"game" / L"csgo" / L"cfg",
        base / L"steamapps" / L"common" / Cs2FolderName / L"game" / L"csgo" / L"cfg"
    };

    for (const fs::path& candidate : candidates)
    {
        if (fs::exists(candidate) && fs::is_directory(candidate))
        {
            const std::wstring lower = Lower(candidate.wstring());
            if (lower.ends_with(L"game\\csgo\\cfg") || lower.ends_with(L"game/csgo/cfg"))
            {
                return candidate.wstring();
            }
        }
    }

    return {};
}

std::wstring RegistryCs2CfgDirectory()
{
    const std::vector<std::pair<HKEY, std::wstring>> appKeys = {
        { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 730" },
        { HKEY_LOCAL_MACHINE, L"Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 730" },
        { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 730" }
    };

    for (const auto& [root, key] : appKeys)
    {
        const std::wstring installLocation = ReadRegistryString(root, key, L"InstallLocation");
        if (installLocation.empty())
        {
            continue;
        }

        const fs::path cfg = fs::path(installLocation) / L"game" / L"csgo" / L"cfg";
        if (fs::exists(cfg))
        {
            return cfg.wstring();
        }
    }

    return {};
}

std::wstring RunningCs2CfgDirectory()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    std::wstring result;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (Lower(entry.szExeFile) != L"cs2.exe")
            {
                continue;
            }

            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
            if (!process)
            {
                continue;
            }

            wchar_t path[MAX_PATH]{};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(process, 0, path, &size))
            {
                fs::path current = fs::path(path).parent_path();
                for (int i = 0; i < 8 && !current.empty(); ++i)
                {
                    const fs::path cfg = current / L"game" / L"csgo" / L"cfg";
                    if (fs::exists(cfg))
                    {
                        result = cfg.wstring();
                        break;
                    }

                    current = current.parent_path();
                }
            }

            CloseHandle(process);
        } while (result.empty() && Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return result;
}

std::wstring FindCs2CfgDirectory(const Settings& settings)
{
    if (!settings.cs2CfgFolderPath.empty() && fs::exists(settings.cs2CfgFolderPath))
    {
        return settings.cs2CfgFolderPath;
    }

    const std::wstring registryCfg = RegistryCs2CfgDirectory();
    if (!registryCfg.empty())
    {
        return registryCfg;
    }

    for (const std::wstring& library : SteamLibraries())
    {
        const fs::path cfg = fs::path(library) / L"steamapps" / L"common" / Cs2FolderName / L"game" / L"csgo" / L"cfg";
        if (fs::exists(cfg))
        {
            return cfg.wstring();
        }
    }

    const std::wstring runningCfg = RunningCs2CfgDirectory();
    if (!runningCfg.empty())
    {
        return runningCfg;
    }

    return {};
}

std::wstring PickCfgDirectory(HWND owner, const Settings& settings)
{
    IFileDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
    {
        return {};
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dialog->SetTitle(Text(settings, L"SelectCfgDescription").c_str());

    std::wstring result;
    std::wstring selectedPath;
    if (SUCCEEDED(dialog->Show(owner)))
    {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)))
        {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
            {
                selectedPath = path;
                result = ResolveCfgDirectory(selectedPath);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }

    dialog->Release();

    if (!selectedPath.empty() && result.empty())
    {
        MessageBoxW(owner, Text(settings, L"InvalidCfgFolder").c_str(), AppName, MB_OK | MB_ICONWARNING);
    }

    return result;
}

std::string BuildGsiConfig(int port)
{
    std::ostringstream config;
    config << "\"CS2RoundAlert\"\n";
    config << "{\n";
    config << "    \"uri\" \"http://127.0.0.1:" << port << "\"\n";
    config << "    \"timeout\" \"5.0\"\n";
    config << "    \"buffer\" \"0.1\"\n";
    config << "    \"throttle\" \"0.5\"\n";
    config << "    \"heartbeat\" \"10.0\"\n";
    config << "    \"data\"\n";
    config << "    {\n";
    config << "        \"round\" \"1\"\n";
    config << "        \"map\"   \"1\"\n";
    config << "    }\n";
    config << "}\n";
    return config.str();
}

std::wstring WriteGsiConfig(const std::wstring& cfg, Settings& settings)
{
    try
    {
        const fs::path configPath = fs::path(cfg) / ConfigFileName;
        WriteText(configPath, BuildGsiConfig(settings.port));
        settings.cs2CfgFolderPath = cfg;
        return Text(settings, L"GsiConfigInstalled") + L" " + configPath.wstring();
    }
    catch (...)
    {
        return Text(settings, L"GsiConfigWriteFailed");
    }
}

std::wstring EnsureGsiConfig(HWND owner, Settings& settings)
{
    (void)owner;

    std::wstring cfg = FindCs2CfgDirectory(settings);
    if (cfg.empty())
    {
        return Text(settings, L"GsiConfigNotInstalled");
    }

    return WriteGsiConfig(cfg, settings);
}

bool IsCs2Foreground()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
    {
        return false;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (!processId)
    {
        return false;
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process)
    {
        return false;
    }

    wchar_t path[MAX_PATH]{};
    DWORD size = MAX_PATH;
    const bool ok = QueryFullProcessImageNameW(process, 0, path, &size) != 0;
    CloseHandle(process);

    if (!ok)
    {
        return false;
    }

    return Lower(fs::path(path).filename().wstring()) == L"cs2.exe";
}

size_t MatchingBrace(const std::string& text, size_t open)
{
    int depth = 0;
    bool inString = false;
    bool escape = false;

    for (size_t i = open; i < text.size(); ++i)
    {
        const char c = text[i];
        if (escape)
        {
            escape = false;
            continue;
        }

        if (c == '\\')
        {
            escape = true;
            continue;
        }

        if (c == '"')
        {
            inString = !inString;
            continue;
        }

        if (inString)
        {
            continue;
        }

        if (c == '{')
        {
            ++depth;
        }
        else if (c == '}')
        {
            --depth;
            if (depth == 0)
            {
                return i;
            }
        }
    }

    return std::string::npos;
}

std::string ExtractRoundPhase(const std::string& body)
{
    const size_t round = body.find("\"round\"");
    if (round == std::string::npos)
    {
        return {};
    }

    const size_t open = body.find('{', round);
    if (open == std::string::npos)
    {
        return {};
    }

    const size_t close = MatchingBrace(body, open);
    if (close == std::string::npos)
    {
        return {};
    }

    const std::string roundObject = body.substr(open, close - open + 1);
    const std::regex phasePattern("\"phase\"\\s*:\\s*\"([^\"]+)\"", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(roundObject, match, phasePattern))
    {
        return match[1].str();
    }

    return {};
}

int ContentLength(const std::string& request)
{
    const std::regex pattern("content-length\\s*:\\s*(\\d+)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(request, match, pattern))
    {
        return std::stoi(match[1].str());
    }

    return 0;
}

class App
{
public:
    int Run(HINSTANCE instance)
    {
        _instance = instance;
        _settings = LoadSettings();

        const wchar_t className[] = L"CS2RoundAlertWindow";
        WNDCLASSW wc{};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = instance;
        wc.hIcon = LoadIconW(nullptr, IDI_INFORMATION);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = className;
        RegisterClassW(&wc);

        _hwnd = CreateWindowExW(
            0,
            className,
            AppName,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            400,
            320,
            nullptr,
            nullptr,
            instance,
            this);
        if (!_hwnd)
        {
            return 1;
        }

        const std::wstring installMessage = EnsureGsiConfig(_hwnd, _settings);
        SaveSettings(_settings);
        AddTrayIcon();
        CreateMainControls();
        SetGsiStatus(Text(_settings, L"NoGsiYet"), false);
        SetLastAction(Text(_settings, L"Listening") + std::to_wstring(_settings.port), false);
        ShowMainWindow();
        StartServer();

        if (!_settings.firstRunNotificationShown)
        {
            ShowBalloon(installMessage, NIIF_INFO);
            _settings.firstRunNotificationShown = true;
            SaveSettings(_settings);
        }

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        StopServer();
        RemoveTrayIcon();
        return 0;
    }

private:
    HINSTANCE _instance{};
    HWND _hwnd{};
    HWND _statusLabel{};
    HWND _enableCheck{};
    HWND _gsiLabel{};
    HWND _lastActionLabel{};
    HWND _chooseButton{};
    HWND _githubButton{};
    HWND _testButton{};
    HWND _hideButton{};
    HWND _quitButton{};
    NOTIFYICONDATAW _tray{};
    Settings _settings;
    std::mutex _settingsMutex;
    std::mutex _statusMutex;
    std::thread _serverThread;
    std::atomic<bool> _running = false;
    SOCKET _listenSocket = INVALID_SOCKET;
    std::string _lastPhase;
    std::wstring _gsiStatusText;
    std::wstring _lastActionText;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            app = reinterpret_cast<App*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }

        if (app)
        {
            return app->HandleMessage(hwnd, message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_TRAYICON && LOWORD(lParam) == WM_RBUTTONUP)
        {
            ShowMenu();
            return 0;
        }

        if (message == WM_TRAYICON && (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == WM_LBUTTONUP))
        {
            ShowMainWindow();
            return 0;
        }

        if (message == WM_STATUS_UPDATE)
        {
            RefreshStatusLabels();
            return 0;
        }

        if (message == WM_CLOSE)
        {
            HideMainWindow();
            return 0;
        }

        if (message == WM_COMMAND)
        {
            HandleCommand(LOWORD(wParam));
            return 0;
        }

        if (message == WM_DESTROY)
        {
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    void CreateMainControls()
    {
        _statusLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE,
            24,
            18,
            340,
            24,
            _hwnd,
            reinterpret_cast<HMENU>(ID_STATUS),
            _instance,
            nullptr);

        _enableCheck = CreateWindowExW(
            0,
            L"BUTTON",
            Text(_settings, L"EnableAlerts").c_str(),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            24,
            46,
            340,
            26,
            _hwnd,
            reinterpret_cast<HMENU>(ID_GUI_ENABLE),
            _instance,
            nullptr);

        _gsiLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE,
            24,
            82,
            340,
            22,
            _hwnd,
            reinterpret_cast<HMENU>(ID_GSI_STATUS),
            _instance,
            nullptr);

        _lastActionLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE,
            24,
            108,
            340,
            22,
            _hwnd,
            reinterpret_cast<HMENU>(ID_LAST_ACTION),
            _instance,
            nullptr);

        _testButton = CreateWindowExW(
            0,
            L"BUTTON",
            Text(_settings, L"TestSound").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            24,
            148,
            160,
            30,
            _hwnd,
            reinterpret_cast<HMENU>(ID_TEST_SOUND),
            _instance,
            nullptr);

        _chooseButton = CreateWindowExW(
            0,
            L"BUTTON",
            Text(_settings, L"ChooseCfgFolder").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            202,
            148,
            160,
            30,
            _hwnd,
            reinterpret_cast<HMENU>(ID_CHOOSE_CFG),
            _instance,
            nullptr);

        _githubButton = CreateWindowExW(
            0,
            L"BUTTON",
            Text(_settings, L"OpenGitHubRepo").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            24,
            192,
            160,
            30,
            _hwnd,
            reinterpret_cast<HMENU>(ID_OPEN_GITHUB),
            _instance,
            nullptr);

        _hideButton = CreateWindowExW(
            0,
            L"BUTTON",
            Text(_settings, L"HideToTray").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            202,
            192,
            160,
            30,
            _hwnd,
            reinterpret_cast<HMENU>(ID_HIDE),
            _instance,
            nullptr);

        _quitButton = CreateWindowExW(
            0,
            L"BUTTON",
            Text(_settings, L"Quit").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            24,
            236,
            338,
            30,
            _hwnd,
            reinterpret_cast<HMENU>(ID_QUIT),
            _instance,
            nullptr);

        RefreshMainControls();
    }

    void RefreshMainControls()
    {
        if (_statusLabel)
        {
            SetWindowTextW(_statusLabel, Text(_settings, _settings.enabled ? L"StatusEnabled" : L"StatusDisabled").c_str());
        }

        if (_enableCheck)
        {
            SetWindowTextW(_enableCheck, Text(_settings, L"EnableAlerts").c_str());
            SendMessageW(_enableCheck, BM_SETCHECK, _settings.enabled ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        RefreshStatusLabels();
        if (_chooseButton) SetWindowTextW(_chooseButton, Text(_settings, L"ChooseCfgFolder").c_str());
        if (_githubButton) SetWindowTextW(_githubButton, Text(_settings, L"OpenGitHubRepo").c_str());
        if (_testButton) SetWindowTextW(_testButton, Text(_settings, L"TestSound").c_str());
        if (_hideButton) SetWindowTextW(_hideButton, Text(_settings, L"HideToTray").c_str());
        if (_quitButton) SetWindowTextW(_quitButton, Text(_settings, L"Quit").c_str());
    }

    void RefreshStatusLabels()
    {
        std::wstring gsiStatus;
        std::wstring lastAction;
        {
            std::lock_guard<std::mutex> lock(_statusMutex);
            gsiStatus = _gsiStatusText;
            lastAction = _lastActionText;
        }

        if (_gsiLabel)
        {
            SetWindowTextW(_gsiLabel, gsiStatus.c_str());
        }

        if (_lastActionLabel)
        {
            SetWindowTextW(_lastActionLabel, lastAction.c_str());
        }
    }

    void SetGsiStatus(const std::wstring& text, bool writeLog = true)
    {
        {
            std::lock_guard<std::mutex> lock(_statusMutex);
            if (_gsiStatusText == text)
            {
                return;
            }
            _gsiStatusText = text;
        }

        if (writeLog)
        {
            AppendLog(text);
        }

        if (_hwnd)
        {
            PostMessageW(_hwnd, WM_STATUS_UPDATE, 0, 0);
        }
    }

    void SetLastAction(const std::wstring& text, bool writeLog = true)
    {
        {
            std::lock_guard<std::mutex> lock(_statusMutex);
            if (_lastActionText == text)
            {
                return;
            }
            _lastActionText = text;
        }

        if (writeLog)
        {
            AppendLog(text);
        }

        if (_hwnd)
        {
            PostMessageW(_hwnd, WM_STATUS_UPDATE, 0, 0);
        }
    }

    void CenterMainWindow()
    {
        RECT rect{};
        GetWindowRect(_hwnd, &rect);
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(_hwnd, nullptr, (screenWidth - width) / 2, (screenHeight - height) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void ShowMainWindow()
    {
        RefreshMainControls();
        CenterMainWindow();
        ShowWindow(_hwnd, SW_SHOWNORMAL);
        SetForegroundWindow(_hwnd);
    }

    void HideMainWindow()
    {
        ShowWindow(_hwnd, SW_HIDE);
    }

    void AddTrayIcon()
    {
        _tray = {};
        _tray.cbSize = sizeof(_tray);
        _tray.hWnd = _hwnd;
        _tray.uID = ID_TRAY;
        _tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        _tray.uCallbackMessage = WM_TRAYICON;
        _tray.hIcon = LoadIconW(nullptr, IDI_INFORMATION);
        wcscpy_s(_tray.szTip, AppName);
        Shell_NotifyIconW(NIM_ADD, &_tray);
    }

    void RemoveTrayIcon()
    {
        Shell_NotifyIconW(NIM_DELETE, &_tray);
    }

    void ShowBalloon(const std::wstring& message, DWORD icon)
    {
        NOTIFYICONDATAW data = _tray;
        data.uFlags = NIF_INFO;
        wcscpy_s(data.szInfoTitle, AppName);
        wcsncpy_s(data.szInfo, message.c_str(), _TRUNCATE);
        data.dwInfoFlags = icon;
        Shell_NotifyIconW(NIM_MODIFY, &data);
    }

    void ShowMenu()
    {
        POINT point{};
        GetCursorPos(&point);

        HMENU menu = CreatePopupMenu();
        HMENU language = CreatePopupMenu();

        const bool enabled = _settings.enabled;
        AppendMenuW(menu, MF_STRING | (enabled ? MF_CHECKED : 0), ID_ENABLE, Text(_settings, L"EnableAlerts").c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, ID_TEST_SOUND, Text(_settings, L"TestSound").c_str());
        AppendMenuW(menu, MF_STRING, ID_OPEN_SETTINGS, Text(_settings, L"OpenSettingsFolder").c_str());
        AppendMenuW(menu, MF_STRING, ID_OPEN_GITHUB, Text(_settings, L"OpenGitHubRepo").c_str());
        AppendMenuW(menu, MF_STRING, ID_CHOOSE_CFG, Text(_settings, L"ChooseCfgFolder").c_str());

        AppendMenuW(language, MF_STRING | (_settings.language == L"auto" ? MF_CHECKED : 0), ID_LANG_AUTO, Text(_settings, L"LanguageAuto").c_str());
        AppendMenuW(language, MF_STRING | (_settings.language == L"en" ? MF_CHECKED : 0), ID_LANG_EN, Text(_settings, L"LanguageEnglish").c_str());
        AppendMenuW(language, MF_STRING | (_settings.language == L"zh-CN" ? MF_CHECKED : 0), ID_LANG_ZH, Text(_settings, L"LanguageChinese").c_str());
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(language), Text(_settings, L"Language").c_str());

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, ID_QUIT, Text(_settings, L"Quit").c_str());

        SetForegroundWindow(_hwnd);
        TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, _hwnd, nullptr);
        DestroyMenu(menu);
    }

    void HandleCommand(UINT command)
    {
        if (command == ID_ENABLE)
        {
            _settings.enabled = !_settings.enabled;
            SaveSettings(_settings);
            RefreshMainControls();
            return;
        }

        if (command == ID_GUI_ENABLE)
        {
            _settings.enabled = SendMessageW(_enableCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            SaveSettings(_settings);
            RefreshMainControls();
            return;
        }

        if (command == ID_OPEN_SETTINGS)
        {
            fs::create_directories(AppDataDirectory());
            ShellExecuteW(nullptr, L"open", AppDataDirectory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }

        if (command == ID_OPEN_GITHUB)
        {
            ShellExecuteW(nullptr, L"open", RepositoryUrl, nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }

        if (command == ID_CHOOSE_CFG)
        {
            const std::wstring cfg = PickCfgDirectory(_hwnd, _settings);
            if (!cfg.empty())
            {
                const std::wstring message = WriteGsiConfig(cfg, _settings);
                SaveSettings(_settings);
                RefreshMainControls();
                ShowBalloon(message, NIIF_INFO);
            }
            return;
        }

        if (command == ID_TEST_SOUND)
        {
            PlayAlertSound();
            SetLastAction(Text(_settings, L"TestSoundPlayed"));
            return;
        }

        if (command == ID_LANG_AUTO || command == ID_LANG_EN || command == ID_LANG_ZH)
        {
            if (command == ID_LANG_AUTO) _settings.language = L"auto";
            if (command == ID_LANG_EN) _settings.language = L"en";
            if (command == ID_LANG_ZH) _settings.language = L"zh-CN";
            SaveSettings(_settings);
            RefreshMainControls();
            return;
        }

        if (command == ID_HIDE)
        {
            HideMainWindow();
            return;
        }

        if (command == ID_QUIT)
        {
            DestroyWindow(_hwnd);
        }
    }

    void StartServer()
    {
        _running = true;
        _serverThread = std::thread([this] { ServerLoop(); });
    }

    void StopServer()
    {
        _running = false;
        if (_listenSocket != INVALID_SOCKET)
        {
            closesocket(_listenSocket);
            _listenSocket = INVALID_SOCKET;
        }

        if (_serverThread.joinable())
        {
            _serverThread.join();
        }
    }

    void ServerLoop()
    {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        {
            SetLastAction(L"Listen failed: WSAStartup");
            return;
        }

        _listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_listenSocket == INVALID_SOCKET)
        {
            SetLastAction(L"Listen failed: socket");
            WSACleanup();
            return;
        }

        BOOL reuse = TRUE;
        setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<u_short>(_settings.port));
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

        if (bind(_listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
            listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            SetLastAction(Text(_settings, L"ListenFailed"));
            ShowBalloon(Text(_settings, L"ListenFailed"), NIIF_ERROR);
            closesocket(_listenSocket);
            _listenSocket = INVALID_SOCKET;
            WSACleanup();
            return;
        }

        SetLastAction(Text(_settings, L"Listening") + std::to_wstring(_settings.port));

        while (_running)
        {
            SOCKET client = accept(_listenSocket, nullptr, nullptr);
            if (client == INVALID_SOCKET)
            {
                continue;
            }

            HandleClient(client);
            closesocket(client);
        }

        WSACleanup();
    }

    void HandleClient(SOCKET client)
    {
        std::string request;
        char buffer[4096]{};

        while (request.find("\r\n\r\n") == std::string::npos && request.size() < 65536)
        {
            const int received = recv(client, buffer, sizeof(buffer), 0);
            if (received <= 0)
            {
                break;
            }
            request.append(buffer, received);
        }

        const size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
            const int contentLength = ContentLength(request);
            const size_t bodyStart = headerEnd + 4;
            while (request.size() < bodyStart + static_cast<size_t>(contentLength) && request.size() < 1024 * 1024)
            {
                const int received = recv(client, buffer, sizeof(buffer), 0);
                if (received <= 0)
                {
                    break;
                }
                request.append(buffer, received);
            }

            if (request.size() >= bodyStart)
            {
                ProcessPayload(request.substr(bodyStart));
            }
        }

        const char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
        send(client, response, sizeof(response) - 1, 0);
    }

    void ProcessPayload(const std::string& body)
    {
        const std::string phase = ExtractRoundPhase(body);
        if (phase.empty())
        {
            SetGsiStatus(Text(_settings, L"GsiNoRound"));
            return;
        }

        SetGsiStatus(Text(_settings, L"GsiPhase") + Utf8ToWide(phase));
        const bool enteredFreezetime = _stricmp(phase.c_str(), "freezetime") == 0 && _stricmp(_lastPhase.c_str(), "freezetime") != 0;
        _lastPhase = phase;

        if (enteredFreezetime)
        {
            Alert();
            return;
        }

        SetLastAction(Text(_settings, L"WaitingFreezetime"), false);
    }

    void PlayAlertSound()
    {
        if (!_settings.useSystemSound && !_settings.customWavPath.empty() && fs::exists(_settings.customWavPath))
        {
            PlaySoundW(_settings.customWavPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
            return;
        }

        if (!PlaySoundW(L"SystemExclamation", nullptr, SND_ALIAS | SND_ASYNC))
        {
            MessageBeep(MB_ICONEXCLAMATION);
        }
    }

    void Alert()
    {
        if (!_settings.enabled)
        {
            SetLastAction(Text(_settings, L"AlertSkippedDisabled"));
            return;
        }

        if (_settings.alertOnlyWhenNotFocused && IsCs2Foreground())
        {
            SetLastAction(Text(_settings, L"AlertSkippedForeground"));
            return;
        }

        PlayAlertSound();
        SetLastAction(Text(_settings, L"AlertPlayed"));
    }
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    App app;
    const int result = app.Run(instance);
    CoUninitialize();
    return result;
}
