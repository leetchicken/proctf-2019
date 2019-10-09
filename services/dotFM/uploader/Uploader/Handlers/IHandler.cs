using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;

namespace Uploader.Handlers
{
    public interface IHandler
    {
        Task HandleRequest(HttpContext context);
    }
}