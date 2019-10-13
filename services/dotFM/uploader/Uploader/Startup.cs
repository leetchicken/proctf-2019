using System;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.DependencyInjection;
using Uploader.Handlers;
using Uploader.Storages;
using Uploader.Unpackers;

namespace Uploader
{
    public class Startup
    {
        // This method gets called by the runtime. Use this method to add services to the container.
        // For more information on how to configure your application, visit https://go.microsoft.com/fwlink/?LinkID=398940
        public void ConfigureServices(IServiceCollection services)
        {
        }

        // This method gets called by the runtime. Use this method to configure the HTTP request pipeline.
        public void Configure(IApplicationBuilder app, IWebHostEnvironment env)
        {
            var unpacker = new Unpacker();
            var storage = new Storage(env.ContentRootPath, () => TimeSpan.FromMinutes(30));

            IHandler playlistUnpacker = new PlaylistUnpacker(unpacker, storage);
            IHandler playlistGetter = new GetPlaylistHandler(storage);

            IHandler otherRequestsHandler = new OtherRequestsHandler();

            var router = new Router(tuple =>
                (tuple.method.ToUpper(), tuple.path.ToLower()) switch
                {
                    ("POST", "/playlist") => playlistUnpacker,
                    ("GET", "/playlist") => playlistGetter,
                    _ => otherRequestsHandler
                });
            
            app.Run(router.HandleRequest);
        }
    }
}