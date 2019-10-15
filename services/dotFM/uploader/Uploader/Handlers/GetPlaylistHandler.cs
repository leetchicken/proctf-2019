using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Uploader.Extensions;
using Uploader.Storages;

namespace Uploader.Handlers
{
    public class GetPlaylistHandler: IHandler
    {
        private readonly IStorage storage;

        public GetPlaylistHandler(IStorage storage) => 
            this.storage = storage;

        public async Task HandleRequest(HttpContext context)
        {
            var playlistId = context.Request.Query["playlist_id"];
            var playlistContent = storage.Get(Guid.Parse(playlistId));
            if (playlistContent == null)
            {
                await context.SendTextResponse(StatusCodes.Status404NotFound, "");
            }
            else
            {
                var obj = JsonSerializer.SerializeToUtf8Bytes(playlistContent, typeof(List<string>));
                var result = $"{{\"tracks\": {Encoding.UTF8.GetString(obj)}}}";
                await context.SendOK(result);
            }
        }
    }
}