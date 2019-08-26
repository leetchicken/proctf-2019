using System;
using System.Threading;
using System.Threading.Tasks;
using EasyNetQ;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using RabbitMQ.Client;

namespace Deer
{
    public class RabbitMqService : IHostedService
    {
        private const string UsersQueueName = "users.log-processor";

        private readonly IBus _bus;
        private readonly ILogConsumerService _logConsumerService;
        private readonly ILogger<RabbitMqService> _logger;
        private IDisposable _consumer;

        public RabbitMqService(IBus bus, ILogConsumerService logConsumerService, ILogger<RabbitMqService> logger)
        {
            _bus = bus;
            _logConsumerService = logConsumerService;
            _logger = logger;
        }

        public async Task StartAsync(CancellationToken cancellationToken)
        {
            await _logConsumerService.StartAsync();
            
            var exchange = await _bus.Advanced.ExchangeDeclareAsync("users", ExchangeType.Fanout);
            var queue = await _bus.Advanced.QueueDeclareAsync(UsersQueueName);
            await _bus.Advanced.BindAsync(exchange, queue, "added");
            _consumer = _bus.Advanced.Consume<string>(queue, (m, mri) => _logConsumerService.AddConsumerForUserAsync(m.Body));

            _logger.LogInformation($"{nameof(RabbitMqService)} started.");
        }

        public async Task StopAsync(CancellationToken cancellationToken)
        {
            _consumer?.Dispose();
            await _logConsumerService.StopAsync();

            _logger.LogInformation($"{nameof(RabbitMqService)} stopped.");
        }
    }
}