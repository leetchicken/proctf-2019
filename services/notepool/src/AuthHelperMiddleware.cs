using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;

namespace notepool
{
	public class AuthHelperMiddleware
	{
		public AuthHelperMiddleware(RequestDelegate next) => this.next = next;

		public async Task Invoke(HttpContext context)
		{
			context.Response.OnStarting(() =>
			{
				UpdateCookie(context);
				return Task.CompletedTask;
			});
			await next.Invoke(context);
		}

		public static void UpdateCookie(HttpContext context, string login = null)
		{
			login ??= context.User?.Identity?.Name;
			if(context.Request.Cookies.TryGetValue(LoginCookieName, out var lgn) && lgn != null && login == null)
				context.Response.Cookies.Delete(LoginCookieName);
			else if(lgn != login)
				context.Response.Cookies.Append(LoginCookieName, login);
		}

		private const string LoginCookieName = "lgn";
		private readonly RequestDelegate next;
	}
}
