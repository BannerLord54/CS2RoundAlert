using System.Globalization;
using CS2RoundAlert.Settings;

namespace CS2RoundAlert.Localization;

internal sealed class LocalizationService
{
    public const string AutoLanguage = "auto";
    public const string EnglishLanguage = "en";
    public const string ChineseLanguage = "zh-CN";

    private static readonly Dictionary<string, (string En, string Zh)> Strings = new()
    {
        ["EnableAlerts"] = ("Enable alerts", "开启提示音"),
        ["OpenSettingsFolder"] = ("Open settings folder", "打开设置文件夹"),
        ["OpenGitHubRepo"] = ("Open GitHub repo", "打开 GitHub 仓库"),
        ["Language"] = ("Language", "语言"),
        ["LanguageAuto"] = ("Auto (system)", "自动（跟随系统）"),
        ["LanguageEnglish"] = ("English", "English"),
        ["LanguageChinese"] = ("中文", "中文"),
        ["Quit"] = ("Quit", "退出"),
        ["ListenError"] = ("Could not listen on 127.0.0.1:{0}. {1}", "无法监听 127.0.0.1:{0}。{1}"),
        ["GsiConfigNotInstalled"] = ("GSI config was not installed. Restart the app to choose the CS2 cfg folder.", "未安装 GSI 配置。请重启程序并选择 CS2 cfg 文件夹。"),
        ["GsiConfigInstalled"] = ("GSI config installed at: {0}", "GSI 配置已安装到：{0}"),
        ["GsiConfigWriteFailed"] = ("Could not write the GSI config: {0}", "无法写入 GSI 配置：{0}"),
        ["SelectCfgDescription"] = ("Select the CS2 cfg folder or the Counter-Strike Global Offensive folder.", "选择 CS2 cfg 文件夹，或选择 Counter-Strike Global Offensive 文件夹。"),
        ["InvalidCfgFolder"] = ("That folder does not look like the CS2 cfg folder. Select Counter-Strike Global Offensive\\game\\csgo\\cfg.", "这个文件夹不像 CS2 cfg 文件夹。请选择 Counter-Strike Global Offensive\\game\\csgo\\cfg。")
    };

    private readonly AppSettings _settings;

    public LocalizationService(AppSettings settings)
    {
        _settings = settings;
    }

    public string Text(string key)
    {
        if (!Strings.TryGetValue(key, out var value))
        {
            return key;
        }

        return IsChinese() ? value.Zh : value.En;
    }

    public string Format(string key, params object[] args)
    {
        return string.Format(CultureInfo.CurrentCulture, Text(key), args);
    }

    public bool IsLanguageChecked(string language)
    {
        return NormalizeLanguage(_settings.Language) == language;
    }

    private bool IsChinese()
    {
        var language = NormalizeLanguage(_settings.Language);
        if (language == ChineseLanguage)
        {
            return true;
        }

        if (language == EnglishLanguage)
        {
            return false;
        }

        return CultureInfo.CurrentUICulture.TwoLetterISOLanguageName.Equals("zh", StringComparison.OrdinalIgnoreCase);
    }

    private static string NormalizeLanguage(string? language)
    {
        return language switch
        {
            EnglishLanguage => EnglishLanguage,
            ChineseLanguage => ChineseLanguage,
            _ => AutoLanguage
        };
    }
}
