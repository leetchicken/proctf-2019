using System.Threading.Tasks;
using IdentityServer.Models;

namespace IdentityServer.Repositories
{
    public interface IUserRepository
    {
        Task<User> GetAsync(string username);
        
        Task CreateAsync(User user);
    }
}