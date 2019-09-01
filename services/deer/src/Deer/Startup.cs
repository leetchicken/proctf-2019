using System;
using System.Linq;
using Deer.EasyNetQ;
using Deer.Hangfire;
using Deer.Models.MongoDb;
using Deer.Repositories;
using EasyNetQ;
using EasyNetQ.ConnectionString;
using EasyNetQ.Consumer;
using EasyNetQ.Logging;
using EasyNetQ.Management.Client;
using Hangfire;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using MongoDB.Driver;

namespace Deer
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

            services.AddSingleton(sp => new ConnectionStringParser().Parse(sp.GetRequiredService<IConfiguration>().GetConnectionString("RabbitMq")));
            
            services.AddSingleton(sp => RabbitHutch.CreateBus(sp.GetRequiredService<IConfiguration>().GetConnectionString("RabbitMq"),
                sr =>
                {
                    sr.Register<IConsumerErrorStrategy>(srr =>
                        new ConsumerErrorStrategy(sp.GetRequiredService<IUserRepository>(),
                            srr.Resolve<IConnectionFactory>(),
                            srr.Resolve<ITypeNameSerializer>(),
                            sp.GetRequiredService<ILogger<ConsumerErrorStrategy>>()));
                }));

            services.AddSingleton<IManagementClient>(sp =>
            {
                var connectionConfiguration = sp.GetRequiredService<ConnectionConfiguration>();
                return new ManagementClient($"http://{connectionConfiguration.Hosts.First().Host}", connectionConfiguration.UserName, connectionConfiguration.Password);
            });

            services.AddSingleton<IMongoClient>(sp => new MongoClient(sp.GetRequiredService<IConfiguration>().GetConnectionString("MongoDb")));
            services.AddSingleton(sp =>
                sp.GetRequiredService<IMongoClient>().GetDatabase(new MongoUrl(sp.GetRequiredService<IConfiguration>().GetConnectionString("MongoDb")).DatabaseName));

            services.AddSingleton(sp => sp.GetRequiredService<IMongoDatabase>().GetCollection<UserMongoDocument>("users"));
            services.AddSingleton<IUserRepository, UserRepository>();

            services.AddSingleton<IOpenDistroElasticsearchClient>(sp =>
                new OpenDistroElasticsearchClient(new Uri(sp.GetRequiredService<IConfiguration>().GetConnectionString("ElasticSearch"), UriKind.Absolute),
                    sp.GetRequiredService<ILogger<OpenDistroElasticsearchClient>>()));
            services.AddSingleton<ILogRepository>(sp =>
                new LogRepository(new Uri(sp.GetRequiredService<IConfiguration>().GetConnectionString("ElasticSearch"), UriKind.Absolute)));

            services.AddSingleton<IUserUnitOfWork, UserUnitOfWork>();
            services.AddSingleton<ILogConsumerService, LogConsumerService>();
            services.AddSingleton<DeleteOldUsersJob>();

            services.AddAuthentication(CookieAuthenticationDefaults.AuthenticationScheme).AddCookie();

            services.AddMvc().SetCompatibilityVersion(CompatibilityVersion.Version_2_2);
            
            LogProvider.IsDisabled = true;
            
            services.AddHostedService<LogConsumerService>();
            services.AddHostedService<HangfireService>();
        }

        public void Configure(IApplicationBuilder app, IHostingEnvironment env)
        {
            if (env.IsDevelopment())
            {
                app.UseDeveloperExceptionPage();
            }

            GlobalConfiguration.Configuration.UseActivator(new ContainerJobActivator(app.ApplicationServices));
            
            app.UseStaticFiles();
            app.UseAuthentication();
            app.UseMvcWithDefaultRoute();
        }
    }
}