using System.Threading.Tasks;

namespace Deer
{
    public interface ILogConsumerService
    {
        Task AddConsumerForUserAsync(string username);
        Task RemoveConsumerForUserAsync(string username);
    }
}