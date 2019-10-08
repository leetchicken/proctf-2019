using System.Collections.Generic;
using System.IO;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.DataProtection;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.FileProviders;

namespace notepool
{
	public class Startup
	{
		public void ConfigureServices(IServiceCollection services)
		{
			services.AddDataProtection().PersistKeysToFileSystem(new DirectoryInfo("data/settings")).SetApplicationName(nameof(notepool));
			services
				.AddAuthentication(CookieAuthenticationDefaults.AuthenticationScheme)
				.AddCookie(CookieAuthenticationDefaults.AuthenticationScheme, options =>
				{
					options.Cookie.SameSite = SameSiteMode.Strict;
					options.Cookie.Name = "usr";
				});
			services.AddMvc();
			services.AddControllers();
		}

		public void Configure(IApplicationBuilder app, IHostingEnvironment env, IApplicationLifetime applicationLifetime)
		{
			var provider = new PhysicalFileProvider(Path.GetFullPath("wwwroot"));
			applicationLifetime.ApplicationStopping.Register(LuceneIndex.Close);
			app
				.UseDefaultFiles(new DefaultFilesOptions {DefaultFileNames = new List<string> {"index.html"}, FileProvider = provider})
				.UseStaticFiles(new StaticFileOptions {FileProvider = provider})
				.UseAuthentication()
				.UseRouting()
				.UseEndpoints(endpoints => endpoints.MapControllers())
				.Run(async context =>
				{
					context.Response.StatusCode = 404;
					await context.Response.WriteAsync("meow?");
				});
		}
	}
}
