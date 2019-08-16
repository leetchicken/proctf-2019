namespace rubik

open Microsoft.AspNetCore.Mvc

open Helper
open TokenHelper
open rubikdb

[<ApiController>]
type AuthHandler() =
    inherit ControllerBase()

    [<HttpGet("/api/auth")>]
    member __.GetUser() =
        let key = stackalloc<byte>(SettingsManager.Current.KeySize)
        SettingsManager.Current.Key.TryWriteBytes(key) |> ignore

        match RubikDb.FindUser(base.HttpContext.FindAuthLogin(key.AsReadOnly())) with
        | user when user <> null -> base.Ok({| Login = user.Login; Bio = user.Bio; Created = user.Created |}) :> IActionResult
        | _ -> base.Unauthorized() :> IActionResult
