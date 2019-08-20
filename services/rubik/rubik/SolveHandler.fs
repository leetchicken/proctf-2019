namespace rubik

open System
open Microsoft.AspNetCore.Http
open Microsoft.AspNetCore.Mvc

open Rubik
open Helper
open TokenHelper
open rubikdb

[<ApiController>]
type SolveHandler() =
    inherit ControllerBase()

    static let MaxFieldLength = 32
    static let MaxSolutionLength = 1024

    static member TryGetMoves(solution: string, moves: Span<Rotation>) : bool =
        let mutable flag = true
        for i = 0 to solution.Length - 1 do
            moves.[i] <- match Enum.TryParse(solution.[i].ToString()) with
            | true, rotation -> rotation
            | _ ->
                flag <- false
                Rotation.L
        flag

    static member GetOrAddUser(context: HttpContext, login: string, pass: string, bio: string, key: ReadOnlySpan<byte>) =
        match RubikDb.FindUser(context.FindAuthLogin(key)) with
        | user when user <> null -> (200, null, user)
        | _ ->
            match login.TrimOrNull(), pass.TrimOrNull(), bio.TrimOrNull() with
            | login, pass, bio when String.IsNullOrEmpty(login) && String.IsNullOrEmpty(pass) && String.IsNullOrEmpty(bio) ->
                (200, null, new User(Login = "Guest #" + Guid.NewGuid().ToString("N").Substring(0, 12), Created = DateTime.UtcNow))
            | login, pass, bio when String.IsNullOrEmpty(login) || login.Length > MaxFieldLength || String.IsNullOrEmpty(pass) || pass.Length > MaxFieldLength || not(String.IsNullOrEmpty(bio)) && bio.Length > MaxFieldLength ->
                (400, "Bad login or pass or bio", null)
            | login, pass, bio ->
                match RubikDb.GetOrAddUser(new User(Login = login, Pass = pass, Bio = bio, Created = DateTime.UtcNow)) with
                | user when user <> null && pass.AsSpan().ConstantTimeEquals(user.Pass.AsSpan(), fun b -> int(b)) ->
                    context.SetAuthCookie(key, user)
                    (200, null, user)
                | _ -> (403, "Invalid pass", null)

    static member Solve(context: HttpContext, puzzle: string, solution: string, login: string, pass: string, bio: string) =
        match puzzle, (if solution <> null then solution else "") with
        | puzzle, solution when not(String.IsNullOrEmpty(puzzle)) && solution.Length <= MaxSolutionLength ->
            match RubikHelper.TryDeserialize(puzzle, SettingsManager.Current.Key) with
            | rubik when rubik.IsEmpty() -> (400, "Bad puzzle", null)
            | rubik ->
                let moves = stackalloc<Rotation>(solution.Length)
                match SolveHandler.TryGetMoves(solution, moves) with
                | false -> (400, "Bad solution", null)
                | true ->
                    let key = stackalloc<byte>(SettingsManager.Current.KeySize)
                    SettingsManager.Current.Key.TryWriteBytes(key) |> ignore

                    rubik.Turn(moves)

#if HACK
                    Console.WriteLine("===")
                    Console.WriteLine(" raw addr: " + (new CastToPointerStruct(rubik.Raw)).Pointer.ToString("x16"))
                    Console.WriteLine(" key addr: " + (new CastToPointerStruct(key)).Pointer.ToString("x16"))
                    Console.WriteLine(" dat addr: " + (new CastToPointerStruct(System.Runtime.InteropServices.MemoryMarshal.Cast<Rotation, byte>(moves))).Pointer.ToString("x16"))
                    Console.WriteLine("     diff: " + ((new CastToPointerStruct(key)).Pointer - (new CastToPointerStruct(rubik.Raw)).Pointer).ToString())
                    Console.WriteLine("      key: " + System.BitConverter.ToString(key.ToArray()))
                    Console.WriteLine("      raw: " + System.BitConverter.ToString(rubik.Raw.ToArray()))
#endif

                    match SolveHandler.GetOrAddUser(context, login, pass, bio, key.AsReadOnly()) with
                    | (200, _, user) when user <> null ->
                        match rubik.IsCompleted() with
                        | true ->
                            let sln = new Solution(Id = rubik.Id, Created = DateTime.UtcNow, Login = user.Login, MovesCount = moves.Length, Time = uint64((DateTime.UtcNow - rubik.Created).TotalMilliseconds))
                            if RubikDb.TryAddSolution(sln) then (200, null, sln) else (409, "Already solved", null)
                        | _ -> (418, "Better luck next time", null)
                    | (status, msg, _) -> (status, msg, null)
        | _ -> (400, "Bad puzzle or solution", null)

    [<HttpPost("/api/solve")>]
    member __.ProcessRequest(puzzle: string, solution: string, login: string, pass: string, bio: string): IActionResult =
        match SolveHandler.Solve(base.HttpContext, puzzle, solution, login, pass, bio) with
        | (200, _, sln) when sln <> null -> base.Ok(sln) :> IActionResult
        | (status, message, _) -> base.StatusCode(status, message) :> IActionResult
