using System.Threading.Tasks;

namespace IdentityServer.Repositories
{
    public interface IOpenDistroElasticsearchClient
    {
        Task CreateUserAsync(string username, string password);
        Task CreateIndexAsync(string index);
    }
}