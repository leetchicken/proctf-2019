using System;
using System.Threading;
using System.Threading.Tasks;
using EasyNetQ;
using LogProcessor.Handlers;
using LogProcessor.Models;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace LogProcessor
{
    public class RabbitMqService : IHostedService
    {
        private const string UsersQueueName = "users.log-processor";

        private readonly IBus _bus;
        private readonly UserAddedMessageHandler _handler;
        private readonly ILogger<RabbitMqService> _logger;
        private IDisposable _consumer;

        public RabbitMqService(IBus bus, UserAddedMessageHandler handler, ILogger<RabbitMqService> logger)
        {
            _bus = bus;
            _handler = handler;
            _logger = logger;
        }

        public Task StartAsync(CancellationToken cancellationToken)
        {
            var queue = _bus.Advanced.QueueDeclare(UsersQueueName, true);
            _consumer = _bus.Advanced.Consume<UserAddedMessage>(queue, (m, mri) => _handler.HandleAsync(m.Body));

            _logger.LogInformation($"{nameof(RabbitMqService)} started.");
            return Task.CompletedTask;
        }

        public Task StopAsync(CancellationToken cancellationToken)
        {
            _consumer?.Dispose();

            _logger.LogInformation($"{nameof(RabbitMqService)} stopped.");
            return Task.CompletedTask;
        }
    }
}