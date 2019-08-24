using System;
using System.Text;
using System.Threading;
using Deer.Messages;
using EasyNetQ;
using EasyNetQ.Consumer;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using RabbitMQ.Client;
using RabbitMQ.Client.Exceptions;
using IConnectionFactory = EasyNetQ.IConnectionFactory;

namespace LogProcessor
{
    public class ConsumerErrorStrategy : IConsumerErrorStrategy
    {
        private readonly IUserRepository _userRepository;
        private readonly IConnectionFactory _connectionFactory;
        private readonly ITypeNameSerializer _typeNameSerializer;
        private readonly ILogger<ConsumerErrorStrategy> _logger;
        private readonly object _syncLock = new object();
        private IConnection _connection;
        private bool _disposed;
        private bool _disposing;

        public ConsumerErrorStrategy(IUserRepository userRepository, IConnectionFactory connectionFactory, ITypeNameSerializer typeNameSerializer, ILogger<ConsumerErrorStrategy> logger)
        {
            _userRepository = userRepository;
            _connectionFactory = connectionFactory;
            _typeNameSerializer = typeNameSerializer;
            _logger = logger;
        }

        public void Dispose()
        {
            if (_disposed)
                return;
            _disposing = true;
            if (_connection != null)
                _connection.Dispose();
            _disposed = true;
        }

        private void Connect()
        {
            if (_connection != null && _connection.IsOpen)
                return;
            object syncLock = this._syncLock;
            bool lockTaken = false;
            try
            {
                Monitor.Enter(syncLock, ref lockTaken);
                if (_connection != null && _connection.IsOpen || (_disposing || _disposed))
                    return;
                if (_connection != null)
                {
                    try
                    {
                        _connection.Dispose();
                    }
                    catch
                    {
                        if (_connection.CloseReason != null)
                            _logger.LogInformation($"Connection {_connection} has shutdown with reason={_connection.CloseReason.Cause}");
                        else
                            throw;
                    }
                }
                _connection = _connectionFactory.CreateConnection();
                if (!_disposing && !_disposed)
                    return;
                _connection.Dispose();
            }
            finally
            {
                if (lockTaken)
                    Monitor.Exit(syncLock);
            }
        }
        
        public AckStrategy HandleConsumerError(ConsumerExecutionContext context, Exception exception)
        {
            if (!_disposed && !_disposing)
            {
                try
                {
                    var user = _userRepository.GetUserByLogQueueAsync(context.Info.Queue);
                    if (user == null)
                    {
                        _logger.LogWarning($"Can't get user by queue '{context.Info.Queue}'");
                        return AckStrategies.Ack;
                    }

                    Connect();
                    using (var model = _connection.CreateModel())
                    {
                        var messageBody = CreateErrorMessage(context, exception);
                        var properties = model.CreateBasicProperties();
                        properties.Persistent = true;
                        properties.Type = _typeNameSerializer.Serialize(typeof (Error));

                        model.BasicPublish("errors", user.Username, false, properties, messageBody);

                        return AckStrategies.Ack;
                    }
                }
                catch (BrokerUnreachableException unreachableException)
                {
                    _logger.LogError(unreachableException, "Cannot connect to broker while attempting to publish error message");
                }
                catch (OperationInterruptedException interruptedException)
                {
                    _logger.LogError(interruptedException, "Broker connection was closed while attempting to publish error message");
                }
                catch (Exception unexpectedException)
                {
                    _logger.LogError(unexpectedException, "Failed to publish error message");
                }
            }

            return AckStrategies.NackWithRequeue;
        }

        public AckStrategy HandleConsumerCancelled(ConsumerExecutionContext context)
        {
            return AckStrategies.NackWithRequeue;
        }

        private byte[] CreateErrorMessage(ConsumerExecutionContext context, Exception exception)
        {
            var error = new Error
            {
                ErrorMessage = exception.Message,
                RawMessage = context.Body
            };

            return Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(error));
        }
    }
}