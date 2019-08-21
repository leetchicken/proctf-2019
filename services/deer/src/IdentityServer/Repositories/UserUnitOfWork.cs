using System;
using System.Linq;
using System.Threading.Tasks;
using EasyNetQ;
using EasyNetQ.Management.Client;
using EasyNetQ.Management.Client.Model;
using IdentityServer.Helpers;
using Microsoft.Extensions.Logging;
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

            var permissionInfo = new PermissionInfo(rabbitUser, vhost)
                .DenyAllConfigure()
                .SetRead("^amp\\.")
                .SetWrite("^logs\\.");
            await _managementClient.CreatePermissionAsync(permissionInfo);
    
            var exchange = await _managementClient.GetExchangeAsync("feedback", vhost);

            var queueName = (await _bus.Advanced.QueueDeclareAsync("")).Name;
            var queue = await _managementClient.GetQueueAsync(queueName, vhost);
            
            var bindingInfo = new BindingInfo(username);
            await _managementClient.CreateBindingAsync(exchange, queue, bindingInfo);

            await _elasticsearchClient.CreateUserAsync(username, password);
            await _elasticsearchClient.CreateIndexAsync(username);
            
            var salt = CryptoUtils.GenerateSalt();
            var passwordHash = CryptoUtils.ComputeHash(salt, password);
            
            var user = new User
            {
                Username = username,
                Salt = salt,
                PasswordHash = passwordHash,
                LogExchangeName = $"logs.{username}",
                FeedbackQueueName = queue.Name
            };
            await _userRepository.CreateAsync(user);
            return true;
        }
    }
}