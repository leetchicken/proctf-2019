using System.Linq;
using Deer.Repositories;
using Microsoft.Extensions.Logging;

namespace Deer.Hangfire
{
    public class DeleteOldUsersJob
    {
        private readonly IUserRepository _userRepository;
        private readonly IUserUnitOfWork _userUnitOfWork;
        private readonly ILogger<DeleteOldUsersJob> _logger;

        public DeleteOldUsersJob(IUserRepository userRepository, IUserUnitOfWork userUnitOfWork, ILogger<DeleteOldUsersJob> logger)
        {
            _userRepository = userRepository;
            _userUnitOfWork = userUnitOfWork;
            _logger = logger;
        }

        public void Run()
        {
            _logger.LogInformation($"{nameof(DeleteOldUsersJob)} started");
            
            var users = _userRepository.GetOldUsersAsync().Result.ToArray();
            foreach (var user in users)
            {
                _userUnitOfWork.DeleteUserAsync(user).Wait();
            }
            
            _logger.LogInformation($"{nameof(DeleteOldUsersJob)} finished");
        }
    }
}