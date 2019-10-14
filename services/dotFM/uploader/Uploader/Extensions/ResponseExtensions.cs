using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;

namespace Uploader.Extensions
{
    public static class ResponseExtensions
    {
        public static async Task SendTextResponse(this HttpContext context, int code, string text)
        {
            context.Response.StatusCode = code;
            await context.Response.WriteAsync(text);
        }

        public static async Task SendOK(this HttpContext context, string text) => 
            await context.SendTextResponse(StatusCodes.Status200OK, text);
        
    }
}