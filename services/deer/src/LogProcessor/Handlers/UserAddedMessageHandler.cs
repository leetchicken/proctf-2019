using System.Threading.Tasks;
using EasyNetQ;
using EasyNetQ.Topology;
using IdentityServer.Models;

namespace LogProcessor.Handlers
{
    public class UserAddedMessageHandler
    {
        private readonly IBus _bus;

        public UserAddedMessageHandler(IBus bus)
        {
            _bus = bus;
        }

        public async Task HandleAsync(UserAddedMessage message)
        {
            var exchange = await _bus.Advanced.ExchangeDeclareAsync(message.LogExchangeName, ExchangeType.Fanout, true);
            var queue = await _bus.Advanced.QueueDeclareAsync("", durable: false, autoDelete: true);

            await _bus.Advanced.BindAsync(exchange, queue, "");
        }
    }
}