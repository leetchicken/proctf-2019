using System.Threading.Tasks;
using IdentityServer.Models;

namespace IdentityServer.Repositories
{
    public interface IUserRepository
    {
        Task CreateAsync(User user);
    }
}