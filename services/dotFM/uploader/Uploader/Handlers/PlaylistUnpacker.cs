using System;
using System.IO;
using System.Net;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Uploader.Storages;
using Uploader.Unpackers;

namespace Uploader.Handlers
{
    public class PlaylistUnpacker: IHandler
    {
        private readonly IUnpacker unpacker;
        private readonly IStorage storage;

        public PlaylistUnpacker(IUnpacker unpacker, IStorage storage)
        {
            this.unpacker = unpacker;
            this.storage = storage;
        }
        
        public async Task HandleRequest(HttpContext context)
        {
            // todo c# 8 pattern matching 
            switch (context.Request.Method)
            {
                case "POST" when context.Request.Path == "/playlist":
                    await TrySavePlaylist(context);
                    break;
                case "GET" when context.Request.Path == "/playlist":
                    await TryGetPlaylist(context);
                    break;
            }
        }

        private static async Task TryGetPlaylist(HttpContext context)
        {
            context.Response.StatusCode = (int) HttpStatusCode.OK;
            await context.Response.WriteAsync("OK");
        }

        private async Task TrySavePlaylist(HttpContext context)
        {
            //Console.WriteLine(context.Request.ContentLength.Value);
            var archive_bytes = new byte[context.Request.ContentLength.Value];
            await context.Request.Body.ReadAsync(archive_bytes);

            var unpacked = unpacker.Unpack(new MemoryStream(archive_bytes));
            if (unpacked == null)
            {
                context.Response.StatusCode = (int) HttpStatusCode.BadRequest;
                await context.Response.WriteAsync("Bad formatting!");
            }
            
            storage.Store(unpacked);
            context.Response.StatusCode = (int) HttpStatusCode.OK;
            await context.Response.WriteAsync("saved!");
        }
    }
}