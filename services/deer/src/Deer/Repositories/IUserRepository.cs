using System.Collections.Generic;
using System.Threading.Tasks;
using Deer.Models;

namespace Deer.Repositories
{
    public interface IUserRepository
    {        
        Task CreateAsync(User user);        
        Task DeleteAsync(string username);        
        Task<IEnumerable<User>> GetUsersAsync();
        Task<User> GetUserAsync(string username);
        User GetUserByLogQueueAsync(string logQueueName);
        Task<IEnumerable<User>> GetOldUsersAsync();
    }
}