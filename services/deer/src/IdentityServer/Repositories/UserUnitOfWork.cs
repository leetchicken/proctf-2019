using System;
using System.Linq;
using System.Threading.Tasks;
using EasyNetQ;
using EasyNetQ.Management.Client;
using EasyNetQ.Management.Client.Model;
using IdentityServer.Helpers;
using Microsoft.Extensions.Logging;
using ExchangeType = RabbitMQ.Client.ExchangeType;
using User = IdentityServer.Models.User;

namespace IdentityServer.Repositories
{
    public class UserUnitOfWork : IUserUnitOfWork
    {
        private readonly IManagementClient _managementClient;
        private readonly IUserRepository _userRepository;
        private readonly IBus _bus;
        private readonly IOpenDistroElasticsearchClient _elasticsearchClient;
        private readonly ILogger<UserUnitOfWork> _logger;

        public UserUnitOfWork(IManagementClient managementClient, IUserRepository userRepository, IBus bus, IOpenDistroElasticsearchClient elasticsearchClient, ILogger<UserUnitOfWork> logger)
        {
            _managementClient = managementClient;
            _userRepository = userRepository;
            _bus = bus;
            _elasticsearchClient = elasticsearchClient;
            _logger = logger;
        }

        public async Task<bool> CreateUserAsync(string username, string password)
        {
            var vhost = await _managementClient.GetVhostAsync("/");

            if ((await _managementClient.GetUsersAsync()).Any(u => string.Equals(u.Name, username, StringComparison.InvariantCultureIgnoreCase)))
            {
                _logger.LogInformation($"User {username} already exists");
                return false;
            }
            
            var userInfo = new UserInfo(username, password);
            var rabbitUser = await _managementClient.CreateUserAsync(userInfo);
            _logger.LogInformation($"RabbitMQ user {username} created");

            var logsExchangeName = $"logs.{username}";
            var exchangeInfo = new ExchangeInfo(logsExchangeName, ExchangeType.Fanout);
            await _managementClient.CreateExchangeAsync(exchangeInfo, vhost);
            _logger.LogInformation($"RabbitMQ exchange for user {username} created");
            
            var permissionInfo = new PermissionInfo(rabbitUser, vhost)
                .DenyAllConfigure()
                .SetRead("^amp\\.")
                .SetWrite("^logs\\.");
            await _managementClient.CreatePermissionAsync(permissionInfo);
            _logger.LogInformation($"RabbitMQ permissions for user {username} set");

            var feedbackExchange = await _managementClient.GetExchangeAsync("feedback", vhost);
            
            var feedbackQueueName = (await _bus.Advanced.QueueDeclareAsync("")).Name;
            _logger.LogInformation($"RabbitMQ feedback queue for user {username} created");
            
            var feedbackQueue = await _managementClient.GetQueueAsync(feedbackQueueName, vhost);
            var bindingInfo = new BindingInfo(username);
            await _managementClient.CreateBindingAsync(feedbackExchange, feedbackQueue, bindingInfo);
            _logger.LogInformation($"RabbitMQ feedback queue for user {username} bound to feedback exchange");

            await _elasticsearchClient.CreateUserAsync(username, password);
            _logger.LogInformation($"ElasticSearch user {username} created");
            
            await _elasticsearchClient.CreateIndexAsync(username);
            _logger.LogInformation($"ElasticSearch index for {username} created");
            
            var salt = CryptoUtils.GenerateSalt();
            var passwordHash = CryptoUtils.ComputeHash(salt, password);
            
            var user = new User
            {
                Username = username,
                Salt = salt,
                PasswordHash = passwordHash,
                LogIndexName = username,
                LogExchangeName = logsExchangeName,
                FeedbackQueueName = feedbackQueueName
            };
            await _userRepository.CreateAsync(user);
            _logger.LogInformation($"UserInfo for user {username} saved to MongoDB");

            var usersExchange = await _bus.Advanced.ExchangeDeclareAsync("users", ExchangeType.Fanout);
            await _bus.Advanced.PublishAsync(usersExchange, "added", false, new Message<string>(username));
            _logger.LogInformation($"Message for new user {username} published");
            
            return true;
        }
    }
}