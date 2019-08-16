namespace rubik

open System
open System.IO
open System.Text.RegularExpressions
open Microsoft.Extensions.Configuration
open Microsoft.Extensions.Primitives

type Settings() =
    member val public Key: Guid = Guid.Empty with get, set
    member val public KeySize = 16 with get, set
    member val public ShuffleMovesCount = 101 with get, set

type SettingsManager() =
    [<Literal>] static let SettingsFilename = "settings.ini"

    [<DefaultValue>]
    static val mutable private current: Settings

    [<DefaultValue>]
    static val mutable private ConfigRoot: IConfigurationRoot

    static member private Update() =
        SettingsManager.current <- SettingsManager.ConfigRoot.GetSection("Settings").Get<Settings>()

    static member public Current with get() = SettingsManager.current

    static do
        SettingsManager.ConfigRoot <- (new ConfigurationBuilder())
            .SetBasePath(Directory.GetCurrentDirectory())
            .AddIniFile("settings.ini", false, true)
            .Build()

        ChangeToken.OnChange((fun () -> SettingsManager.ConfigRoot.GetReloadToken()), (fun () ->
            try
                SettingsManager.Update()
                Console.WriteLine("Settings reloaded")
            with
            | _ -> Console.Error.WriteLine("Failed to reload settings"))
        ) |> ignore

        SettingsManager.ConfigRoot.Reload()

        match SettingsManager.current.Key = Guid.Empty with
        | true ->
            let settings = if File.Exists(SettingsFilename) then File.ReadAllText(SettingsFilename) else "[Settings]\n"
            File.WriteAllText(SettingsFilename, Regex.Replace(settings, @"^\s*Key\s*=.*$", "Key = " + Guid.NewGuid().ToString(), RegexOptions.Multiline))
        | _ -> ignore()
