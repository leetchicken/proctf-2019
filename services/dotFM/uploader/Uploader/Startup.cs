using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;
using Uploader.Handlers;
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
        public void Configure(IApplicationBuilder app, IHostingEnvironment env)
        {
            var unpacker = new Unpacker();
            var storage = new Storage();
            var playlistHandler = new PlaylistUnpacker(unpacker, storage);
            var requestDelegate = new RequestDelegate(playlistHandler.HandleRequest);
            app.Run(requestDelegate);
        }
    }
}