module rubik.SecurityHeadersMiddleware

open System
open System.Threading.Tasks
open Microsoft.AspNetCore.Builder
open Microsoft.AspNetCore.Http

open Helper

type SecurityHeadersMiddleware(next: RequestDelegate) =
    let next = next

    static member SetHeaders(context: HttpContext): Task =
        match context.Response.ContentType with
        | null -> ignore()
        | contentType when contentType.StartsWith("text/html", StringComparison.OrdinalIgnoreCase) ->
            context.Response.Headers.["X-Frame-Options"] <- !> "deny"
            context.Response.Headers.["Referrer-Policy"] <- !> "same-origin"
            context.Response.Headers.["X-XSS-Protection"] <- !> "1; mode=block"
        | _ ->
            context.Response.Headers.["X-Content-Type-Options"] <- !> "nosniff"
        Task.CompletedTask

    member __.Invoke(context: HttpContext): Task =
        context.Response.OnStarting(fun () -> SecurityHeadersMiddleware.SetHeaders(context))
        next.Invoke(context)

type IApplicationBuilder with
    member this.UseSecurityHeaders(): IApplicationBuilder =
        this.UseMiddleware<SecurityHeadersMiddleware>()
