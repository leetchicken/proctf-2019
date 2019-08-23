using EasyNetQ;
using EasyNetQ.Consumer;
using LogProcessor.Models;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using MongoDB.Driver;
using NLog.Extensions.Logging;

namespace LogProcessor
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = new HostBuilder()
                .ConfigureAppConfiguration((hostContext, configBuilder) => { configBuilder.AddJsonFile("appsettings.json", false); })
                .ConfigureServices((hostContext, services) =>
                {
                    services.AddLogging();

                    services.AddSingleton(sp => RabbitHutch.CreateBus(sp.GetRequiredService<IConfiguration>().GetConnectionString("RabbitMq"),
                        sr =>
                        {
                            sr.Register<IConsumerErrorStrategy>(srr =>
                                new ConsumerErrorStrategy(sp.GetRequiredService<IUserRepository>(),
                                    srr.Resolve<IConnectionFactory>(),
                                    srr.Resolve<ITypeNameSerializer>(),
                                    sp.GetRequiredService<ILogger<ConsumerErrorStrategy>>()));
                        }));
                    
                    services.AddSingleton<IMongoClient>(sp => new MongoClient(sp.GetRequiredService<IConfiguration>().GetConnectionString("MongoDb")));
                    services.AddSingleton(sp =>
                        sp.GetRequiredService<IMongoClient>().GetDatabase(new MongoUrl(sp.GetRequiredService<IConfiguration>().GetConnectionString("MongoDb")).DatabaseName));

                    services.AddSingleton(sp => sp.GetRequiredService<IMongoDatabase>().GetCollection<UserMongoDocument>("users"));
                    
                    services.AddSingleton<IUserRepository, UserRepository>();
                    services.AddSingleton<ILogConsumerService, LogConsumerService>();
                    
                    services.AddHostedService<RabbitMqService>();
                })
                .ConfigureLogging((hostingContext, logging) =>
                {
                    logging.ClearProviders();
                    logging.AddNLog();
                });

            builder.Build().Run();
        }
    }
}