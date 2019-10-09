using System.Collections.Generic;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Routing;
using Microsoft.AspNetCore.Routing.Tree;

namespace Uploader.Handlers
{
    public class RouteHandler: IHandler
    {
        private Dictionary<string, IHandler> handlers;
        
        public RouteHandler()
        {
            handlers = new Dictionary<string, IHandler>();
        }

        public void RegisterHandler(string method, string path, IHandler handler)
        {
        }
        
        public async Task HandleRequest(HttpContext context)
        {
            
        }
    }
}