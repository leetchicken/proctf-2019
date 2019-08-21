using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading.Tasks;
using Deer.Messages;
using EasyNetQ;
using EasyNetQ.Topology;

namespace LogProcessor
{
    public class LogConsumerService : ILogConsumerService
    {
        private readonly IBus _bus;
        private readonly ConcurrentDictionary<string, IDisposable> _consumers = new ConcurrentDictionary<string, IDisposable>();

        public LogConsumerService(IBus bus)
        {
            _bus = bus;
        }
        
        public Task StartAsync()
        {
            // TODO load all users
            return Task.CompletedTask;
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
            var logsExchangeName = $"logs.{username}";
            var exchange = await _bus.Advanced.ExchangeDeclareAsync(logsExchangeName, ExchangeType.Fanout, true);
            var queue = await _bus.Advanced.QueueDeclareAsync("", durable: false, autoDelete: true);

            // TODO write errors & oks to feedback exchange
            var consumer = _bus.Advanced.Consume<LogData>(queue, (m, mri) => { });
            _consumers.TryAdd(username, consumer);

            await _bus.Advanced.BindAsync(exchange, queue, "");
        }

        public Task RemoveConsumerForUserAsync(string username)
        {
            _consumers.Remove(username, out var consumer);
            consumer.Dispose();
            return Task.CompletedTask;
        }
    }
}