using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Deer.Messages;
using Deer.Models;
using Deer.Repositories;
using EasyNetQ;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace Deer
{
    public class LogConsumerService : ILogConsumerService, IHostedService
    {
        private readonly IBus _bus;
        private readonly IUserRepository _userRepository;
        private readonly ILogRepository _logRepository;
        private readonly ILogger<LogConsumerService> _logger;
        private readonly ConcurrentDictionary<string, IDisposable> _consumers = new ConcurrentDictionary<string, IDisposable>();

        public LogConsumerService(IBus bus, IUserRepository userRepository, ILogRepository logRepository, ILogger<LogConsumerService> logger)
        {
            _bus = bus;
            _userRepository = userRepository;
            _logRepository = logRepository;
            _logger = logger;
        }
        
        public async Task AddConsumerForUserAsync(string username)
        {
            var user = await _userRepository.GetUserAsync(username);
            _consumers.TryAdd(user.Username, await CreateConsumerAsync(user));
        }

        private async Task<IDisposable> CreateConsumerAsync(User user)
        {
            var queue = await _bus.Advanced.QueueDeclareAsync(user.LogQueueName, passive: true);
            return _bus.Advanced.Consume<LogData>(queue, async (m, mri) =>
            {
                await _logRepository.IndexAsync(user.LogIndexName, m.Body);
            });
        }

        public Task RemoveConsumerForUserAsync(string username)
        {
            _consumers.Remove(username, out var consumer);
            consumer.Dispose();
            return Task.CompletedTask;
        }

        public Task StartAsync(CancellationToken cancellationToken)
        {
            if (!_bus.Advanced.IsConnected)
            {
                async void EventHandler(object o, EventArgs a)
                {
                    await InitConsumersAsync();
                    _bus.Advanced.Connected -= EventHandler;
                }

                _bus.Advanced.Connected += EventHandler;
            }

            _logger.LogInformation($"{nameof(LogConsumerService)} started");
            return Task.CompletedTask;
        }
        
        private async Task InitConsumersAsync()
        {
            _logger.LogInformation("Creating consumers for existing users");
            var users = await _userRepository.GetUsersAsync();
            foreach (var user in users)
            {
                _consumers.TryAdd(user.Username, await CreateConsumerAsync(user));
            }
            _logger.LogInformation("Consumers for existing users created");
        }

        public Task StopAsync(CancellationToken cancellationToken)
        {
            foreach (var consumerPair in _consumers)
            {
                consumerPair.Value.Dispose();
            }
            _logger.LogInformation($"{nameof(LogConsumerService)} stopped");
            return Task.CompletedTask;
        }
    }
}