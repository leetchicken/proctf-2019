using System.Threading.Tasks;

namespace Deer.Repositories
{
    public interface IOpenDistroElasticsearchClient
    {
        Task CreateUserAsync(string username, string password);
        Task DeleteUserAsync(string username);
        Task CreateIndexAsync(string index);
        Task DeleteIndexAsync(string index);
    }
}