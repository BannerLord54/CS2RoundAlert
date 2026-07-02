using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using CS2RoundAlert.Settings;

namespace CS2RoundAlert.Integration;

internal sealed partial class GsiConfigInstaller
{
    private const string ConfigFileName = "gamestate_integration_cs2roundalert.cfg";
    private const string Cs2FolderName = "Counter-Strike Global Offensive";

    public ConfigInstallResult EnsureInstalled(AppSettings settings)
    {
        var cfgDirectory = ResolveRememberedDirectory(settings.Cs2CfgFolderPath)
            ?? TryFindInstalledCfgDirectory()
            ?? PromptForCfgDirectory();

        if (cfgDirectory is null)
        {
            return new ConfigInstallResult(
                false,
                null,
                "GSI config was not installed. Restart the app to choose the CS2 cfg folder.");
        }

        try
        {
            Directory.CreateDirectory(cfgDirectory);
            var configPath = Path.Combine(cfgDirectory, ConfigFileName);
            File.WriteAllText(configPath, BuildConfig(settings.Port), Encoding.ASCII);
            settings.Cs2CfgFolderPath = cfgDirectory;

            return new ConfigInstallResult(
                true,
                configPath,
                $"GSI config installed at: {configPath}");
        }
        catch (Exception ex)
        {
            return new ConfigInstallResult(false, null, $"Could not write the GSI config: {ex.Message}");
        }
    }

    private static string? ResolveRememberedDirectory(string? path)
    {
        return string.IsNullOrWhiteSpace(path) || !Directory.Exists(path) ? null : path;
    }

    private static string? TryFindInstalledCfgDirectory()
    {
        foreach (var libraryPath in ReadSteamLibraryPaths())
        {
            var cfgPath = Path.Combine(libraryPath, "steamapps", "common", Cs2FolderName, "game", "csgo", "cfg");
            if (Directory.Exists(cfgPath))
            {
                return cfgPath;
            }
        }

        return null;
    }

    private static IEnumerable<string> ReadSteamLibraryPaths()
    {
        var defaultSteamPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Steam");

        if (Directory.Exists(defaultSteamPath))
        {
            yield return defaultSteamPath;
        }

        var libraryFoldersPath = Path.Combine(defaultSteamPath, "steamapps", "libraryfolders.vdf");
        if (!File.Exists(libraryFoldersPath))
        {
            yield break;
        }

        string content;
        try
        {
            content = File.ReadAllText(libraryFoldersPath);
        }
        catch
        {
            yield break;
        }

        foreach (Match match in SteamLibraryPathRegex().Matches(content))
        {
            var path = match.Groups["path"].Value.Replace(@"\\", @"\");
            if (Directory.Exists(path))
            {
                yield return path;
            }
        }
    }

    private static string? PromptForCfgDirectory()
    {
        using var dialog = new FolderBrowserDialog
        {
            Description = "Select the CS2 cfg folder or the Counter-Strike Global Offensive folder.",
            ShowNewFolderButton = false,
            UseDescriptionForTitle = true
        };

        if (dialog.ShowDialog() != DialogResult.OK)
        {
            return null;
        }

        var resolved = ResolveCfgDirectoryFromSelection(dialog.SelectedPath);
        if (resolved is not null)
        {
            return resolved;
        }

        MessageBox.Show(
            "That folder does not look like the CS2 cfg folder. Select Counter-Strike Global Offensive\\game\\csgo\\cfg.",
            "CS2RoundAlert",
            MessageBoxButtons.OK,
            MessageBoxIcon.Warning);

        return null;
    }

    private static string? ResolveCfgDirectoryFromSelection(string selectedPath)
    {
        if (!Directory.Exists(selectedPath))
        {
            return null;
        }

        var candidates = new[]
        {
            selectedPath,
            Path.Combine(selectedPath, "game", "csgo", "cfg"),
            Path.Combine(selectedPath, Cs2FolderName, "game", "csgo", "cfg"),
            Path.Combine(selectedPath, "steamapps", "common", Cs2FolderName, "game", "csgo", "cfg")
        };

        foreach (var candidate in candidates)
        {
            if (Directory.Exists(candidate)
                && candidate.EndsWith(Path.Combine("game", "csgo", "cfg"), StringComparison.OrdinalIgnoreCase))
            {
                return candidate;
            }
        }

        return null;
    }

    private static string BuildConfig(int port)
    {
        return $$"""
        "CS2RoundAlert"
        {
            "uri" "http://127.0.0.1:{{port}}"
            "timeout" "5.0"
            "buffer" "0.1"
            "throttle" "0.5"
            "heartbeat" "10.0"
            "data"
            {
                "round" "1"
                "map"   "1"
            }
        }
        """;
    }

    [GeneratedRegex("\"path\"\\s+\"(?<path>[^\"]+)\"", RegexOptions.IgnoreCase)]
    private static partial Regex SteamLibraryPathRegex();
}
