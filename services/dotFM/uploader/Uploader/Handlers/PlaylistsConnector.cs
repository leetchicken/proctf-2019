using System;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Uploader.Extensions;
using Uploader.Storages;

namespace Uploader.Handlers
{
    public class PlaylistsConnector: IHandler
    {
        private readonly IStorage storage;

        public PlaylistsConnector(IStorage storage) => this.storage = storage;

        public async Task HandleRequest(HttpContext context)
        {
            if (context.Request.Headers.TryGetValue("target", out var target) &&
                context.Request.Headers.TryGetValue("source", out var source))
            {
                if (Guid.TryParse(target, out var targetId) && Guid.TryParse(source, out var sourceId))
                {
                    if (storage.Link(sourceId, targetId))
                    {
                        await context.SendOK("");
                    }
                    else
                    {
                        await context.SendTextResponse(StatusCodes.Status404NotFound, "");
                    }
                }
                else
                {
                    await context.SendTextResponse(StatusCodes.Status400BadRequest, "");
                }
            }
            else
            {
                await context.SendTextResponse(StatusCodes.Status400BadRequest, "");
            }
        }
    }
}