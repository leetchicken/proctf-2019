module rubik.TokenHelper

open System
open System.Text
open Microsoft.AspNetCore.Http

open rubik
open Helper
open rubikdb

type HttpContext with
    member this.FindAuthLogin(key: ReadOnlySpan<byte>): string =
        let cookie = this.Request.Cookies.["AUTH"]
        match cookie with
        | cookie when cookie <> null && cookie.Length > 0 ->
            let mutable plain = alloc<byte>(TokenCrypt.CalcDecryptedLength(cookie.Length))
            try
                TokenCrypt.CheckMacAndDecrypt(key, cookie.AsSpan(), &plain)
                Encoding.UTF8.GetString(plain.AsReadOnly())
            with
            | _ -> null
        | _ -> null

    member this.SetAuthCookie(key: ReadOnlySpan<byte>, user: User) =
        let data = alloc<byte>(Encoding.UTF8.GetByteCount(user.Login))
        let length = Encoding.UTF8.GetBytes(user.Login.AsSpan(), data)
        let mutable cookie = alloc<char>(TokenCrypt.CalcEncryptedLength(length))
        TokenCrypt.EncryptAndMac(key, data.AsReadOnly(), &cookie)
        this.Response.Cookies.Append("AUTH", cookie.ToString(), new CookieOptions(HttpOnly = true, SameSite = SameSiteMode.Strict))
