namespace rubik

open System
open System.Linq
open Microsoft.AspNetCore.Mvc

open rubikdb

[<ApiController>]
type ScoreboardHandler() =
    inherit ControllerBase()

    [<HttpGet("/api/scoreboard")>]
    member __.GetScoreboard(skip, top: int) =
        RubikDb.TakeSolutions().Skip(Math.Max(0, skip)).Take(Math.Max(100, Math.Min(top, 10000)))
