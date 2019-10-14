using System.IO;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Uploader.Extensions;
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
            var bodyStream = new MemoryStream();
            await context.Request.Body.CopyToAsync(bodyStream);
            
            if (unpacker.TryUnpack(out var unpacked, bodyStream))
            {
                var result = storage.Store(unpacked);
                await context.SendOK($"{{\"created\": \"{result}\"}}");
            }
            else
            {
                await context.SendTextResponse(
                    StatusCodes.Status400BadRequest, 
                    "Bad archive formatting");
            }
        }
    }
}