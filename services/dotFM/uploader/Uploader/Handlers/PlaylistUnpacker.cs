using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;

namespace Uploader.Handlers
{
    public class PlaylistUnpacker: IHandler
    {
        public async Task HandleRequest(HttpContext context)
        {
            await context.Response.WriteAsync("sho");
        }
    }
}