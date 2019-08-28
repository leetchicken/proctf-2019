using System.Threading.Tasks;
using Deer.Models;

namespace Deer.Repositories
{
    public interface IUserUnitOfWork
    {
        Task<bool> CreateUserAsync(string username, string password);
        Task DeleteUserAsync(User user);
    }
}