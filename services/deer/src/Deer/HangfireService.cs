using System.Threading;
using System.Threading.Tasks;
using Deer.Hangfire;
using Hangfire;
using Hangfire.MemoryStorage;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace Deer
{
    public class HangfireService : IHostedService
    {
        private readonly ILogger<HangfireService> _logger;
        private HangfireJobServer _server;

        public HangfireService(ILogger<HangfireService> logger)
        {
            _logger = logger;
        }

        public Task StartAsync(CancellationToken cancellationToken)
        {
            var storage = new MemoryStorage();
            GlobalConfiguration.Configuration.UseStorage(storage);
            _server = new HangfireJobServer(storage);
            RecurringJob.AddOrUpdate<DeleteOldUsersJob>(nameof(DeleteOldUsersJob), j => j.Run(), Cron.Minutely);
            _logger.LogInformation($"{nameof(HangfireService)} started");
            return Task.CompletedTask;
        }
        
        public Task StopAsync(CancellationToken cancellationToken)
        {
            _server.Dispose();
            _logger.LogInformation($"{nameof(HangfireService)} stopped");
            return Task.CompletedTask;
        }
    }
}