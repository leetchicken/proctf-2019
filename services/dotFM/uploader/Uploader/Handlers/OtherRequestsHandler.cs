using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Uploader.Extensions;

namespace Uploader.Handlers
{
    public class OtherRequestsHandler: IHandler
    {
        public async Task HandleRequest(HttpContext context) => 
            await context.SendTextResponse(StatusCodes.Status400BadRequest, "");
    }
}