using EasyNetQ;
using IdentityServer.Models;
using IdentityServer.Repositories;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using MongoDB.Driver;

namespace IdentityServer
{
    public class Startup
    {
        public void ConfigureServices(IServiceCollection services)
        {
            services.AddSingleton<IConfiguration>(sp =>
            {
                var env = sp.GetRequiredService<IHostingEnvironment>();
                return new ConfigurationBuilder().SetBasePath(env.ContentRootPath).AddJsonFile("appsettings.json", false).Build();
            });

            services.AddSingleton(sp => RabbitHutch.CreateBus(sp.GetRequiredService<IConfiguration>().GetConnectionString("RabbitMq")));

            services.AddSingleton<IMongoClient>(sp => new MongoClient(sp.GetRequiredService<IConfiguration>().GetConnectionString("MongoDb")));
            services.AddSingleton(sp =>
                sp.GetRequiredService<IMongoClient>().GetDatabase(new MongoUrl(sp.GetRequiredService<IConfiguration>().GetConnectionString("MongoDb")).DatabaseName));

            services.AddSingleton(sp => sp.GetRequiredService<IMongoDatabase>().GetCollection<UserMongoDocument>("users"));
            services.AddSingleton<IUserRepository, UserRepository>();

            services.AddAuthentication(CookieAuthenticationDefaults.AuthenticationScheme).AddCookie();

            services.AddMvc().SetCompatibilityVersion(CompatibilityVersion.Version_2_2);
        }

        public void Configure(IApplicationBuilder app, IHostingEnvironment env)
        {
            if (env.IsDevelopment())
                app.UseDeveloperExceptionPage();

            app.UseStaticFiles();

            app.UseAuthentication();

            app.UseMvcWithDefaultRoute();
        }
    }
}