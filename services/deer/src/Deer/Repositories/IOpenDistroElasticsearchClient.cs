using System.Threading.Tasks;

namespace Deer.Repositories
{
    public interface IOpenDistroElasticsearchClient
    {
        Task CreateUserAsync(string username, string password);
        Task CreateIndexAsync(string index);
    }
}