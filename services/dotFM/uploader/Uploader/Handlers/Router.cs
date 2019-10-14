using System;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;

namespace Uploader.Handlers
{
    public class Router: IHandler
    {
        private readonly Func<(string, string), IHandler> routeFunc;
        
        public Router(Func<(string method, string path), IHandler> routeFunc) 
            => this.routeFunc = routeFunc;

        public Task HandleRequest(HttpContext context)
            => routeFunc((context.Request.Method, context.Request.Path)).HandleRequest(context);
    }
}