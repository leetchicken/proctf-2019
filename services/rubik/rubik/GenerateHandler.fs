namespace rubik

open System.Text
open Microsoft.AspNetCore.Mvc

open Helper
open Rubik

[<ApiController>]
type GenerateHandler() =
    inherit ControllerBase()

    [<HttpGet("/api/generate")>]
    member __.Generate() =
        let rubik = Rubik.Init().Shuffle(SettingsManager.Current.ShuffleMovesCount)
        {| rubik = Encoding.ASCII.GetString(rubik.Raw.AsReadOnly()); value = RubikHelper.Serialize(rubik, SettingsManager.Current.Key) |}
