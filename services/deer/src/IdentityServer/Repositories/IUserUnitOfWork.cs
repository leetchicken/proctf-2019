using System.Threading.Tasks;

namespace IdentityServer.Repositories
{
    public interface IUserUnitOfWork
    {
        Task<bool> CreateUserAsync(string username, string password);
    }
}