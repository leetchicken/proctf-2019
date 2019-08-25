namespace rubik

open System
open System.IO
open Microsoft.AspNetCore.Hosting
open Microsoft.Extensions.Configuration
open Microsoft.Extensions.Hosting
open Microsoft.AspNetCore.Server.Kestrel.Core

open rubikdb

module Program =
    let exitCode = 0

    let CreateHostBuilder args =
        Host.CreateDefaultBuilder(args)
            .ConfigureWebHostDefaults(fun builder ->
                builder.UseStartup<Startup>() |> ignore
            )

    type IWebHostBuilder with
        member this.UseConfigurationSection(configuration: IConfiguration) : IWebHostBuilder =
            for setting in configuration.AsEnumerable(true) do
                this.UseSetting(setting.Key, setting.Value) |> ignore
            this

    [<EntryPoint>]
    let main _ =
#if DEBUG
        Rubik.Rubik.Test() |> ignore
#endif

        RubikDb.Init()

        (new WebHostBuilder())
            //.UseConfigurationSection(SettingsManager.ConfigRoot.GetSection("Host"))
            .UseKestrel(fun _ options ->
                options.ListenLocalhost(5001, fun listenOptions ->
                    listenOptions.Protocols <- HttpProtocols.Http2
                    listenOptions.UseHttps(Path.Combine(Directory.GetCurrentDirectory(), "localhost.pfx"), "1234") |> ignore
                )
                options.AddServerHeader <- false
                options.Limits.KeepAliveTimeout <- TimeSpan.FromSeconds(30.0)
                options.Limits.MaxRequestBodySize <- new Nullable<int64>(0L)
                options.Limits.MaxRequestLineSize <- 2048
                options.Limits.MaxRequestHeaderCount <- 32
                options.Limits.MaxRequestHeadersTotalSize <- 8192
                options.Limits.RequestHeadersTimeout <- TimeSpan.FromSeconds(3.0)
            )
            .UseStartup<Startup>()
            .Build()
            .Run() |> ignore

        RubikDb.Close()

        exitCode
