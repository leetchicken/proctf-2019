using EasyNetQ;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
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

                    services.AddSingleton(sp => RabbitHutch.CreateBus(sp.GetRequiredService<IConfiguration>().GetConnectionString("RabbitMq")));
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