using System.Threading.Tasks;

namespace Deer.Repositories
{
    public interface IUserUnitOfWork
    {
        Task<bool> CreateUserAsync(string username, string password);
    }
}