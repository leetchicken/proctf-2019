using System.Collections.Generic;
using System.Threading.Tasks;
using LogProcessor.Models;

namespace LogProcessor
{
    public interface IUserRepository
    {
        Task<IEnumerable<User>> GetUsersAsync();
        Task<User> GetUserAsync(string username);
        User GetUserByLogQueueAsync(string logQueueName);
    }
}