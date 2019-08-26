using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading.Tasks;
using Deer.Messages;
using Deer.Models;
using Deer.Repositories;
using EasyNetQ;

namespace Deer
{
    public class LogConsumerService : ILogConsumerService
    {
        private readonly IBus _bus;
        private readonly IUserRepository _userRepository;
        private readonly ILogRepository _logRepository;
        private readonly ConcurrentDictionary<string, IDisposable> _consumers = new ConcurrentDictionary<string, IDisposable>();

        public LogConsumerService(IBus bus, IUserRepository userRepository, ILogRepository logRepository)
        {
            _bus = bus;
            _userRepository = userRepository;
            _logRepository = logRepository;
        }
        
        public async Task StartAsync()
        {
            var users = await _userRepository.GetUsersAsync();
            foreach (var user in users)
            {
                await ConsumeUserAsync(user);
            }
        }

        public Task StopAsync()
        {
            foreach (var consumerPair in _consumers)
            {
                consumerPair.Value.Dispose();
            }

            return Task.CompletedTask;
        }

        public async Task AddConsumerForUserAsync(string username)
        {
            var user = await _userRepository.GetUserAsync(username);
            await ConsumeUserAsync(user);
        }

        private async Task ConsumeUserAsync(User user)
        {
            var queue = await _bus.Advanced.QueueDeclareAsync(user.LogQueueName, passive: true);
            _consumers.TryAdd(user.Username, _bus.Advanced.Consume<LogData>(queue, async (m, mri) =>
            {
                await _logRepository.IndexAsync(user.LogIndexName, m.Body);
            }));
        }

        public Task RemoveConsumerForUserAsync(string username)
        {
            _consumers.Remove(username, out var consumer);
            consumer.Dispose();
            return Task.CompletedTask;
        }
    }
}