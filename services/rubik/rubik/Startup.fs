namespace rubik

open System.IO
open Microsoft.AspNetCore.Builder
open Microsoft.AspNetCore.Hosting
open Microsoft.AspNetCore.Http
open Microsoft.AspNetCore.StaticFiles
open Microsoft.Extensions.DependencyInjection
open Microsoft.Extensions.Configuration
open Microsoft.Extensions.FileProviders

open Helper
open SecurityHeadersMiddleware

type Startup private () =
    new (configuration: IConfiguration) as this =
        Startup() then
        this.Configuration <- configuration

    member val Configuration: IConfiguration = null with get, set

    member __.ConfigureServices(services: IServiceCollection) =
        services.AddControllers() |> ignore

    member __.Configure(app: IApplicationBuilder, _: IWebHostEnvironment) =
        app
            .UseSecurityHeaders()
            .UseDefaultFiles(
                new DefaultFilesOptions(
                    FileProvider = new PhysicalFileProvider(Path.Combine(Directory.GetCurrentDirectory(), "wwwroot/")),
                    DefaultFileNames = [| "index.html" |]
                )
            )
            .UseStaticFiles(
                new StaticFileOptions(
                    FileProvider = new PhysicalFileProvider(Path.Combine(Directory.GetCurrentDirectory(), "wwwroot/")),
                    ContentTypeProvider = new FileExtensionContentTypeProvider(
                        dict [
                            (".txt", "text/plain; charset=utf-8");

                            (".htm", "text/html; charset=utf-8");
                            (".html", "text/html; charset=utf-8");

                            (".json", "application/json; charset=utf-8");

                            (".css", "text/css; charset=utf-8");
                            (".js", "application/javascript; charset=utf-8");

                            (".svg", "image/svg+xml");
                            (".gif", "image/gif");
                            (".png", "image/png");
                            (".jpg", "image/jpeg");
                            (".jpeg", "image/jpeg");
                            (".webp", "image/webp");

                            (".woff", "application/font-woff");
                            (".woff2", "application/font-woff2");

                            (".ico", "image/x-icon")]
                    ),
                    ServeUnknownFileTypes = false,
                    OnPrepareResponse = fun ctx -> ctx.Context.Response.Headers.["Cache-Control"] <- !> "public, max-age=2419200"
                )
            )
            .UseRouting()
            .UseEndpoints(fun endpoints -> endpoints.MapControllers() |> ignore)
            .Run(fun ctx ->
                ctx.Response.StatusCode <- 404
                ctx.Response.WriteAsync("Not Found")
            ) |> ignore
