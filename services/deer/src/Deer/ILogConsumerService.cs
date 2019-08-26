using System.Threading.Tasks;

namespace Deer
{
    public interface ILogConsumerService
    {
        Task StartAsync();
        Task StopAsync();
        Task AddConsumerForUserAsync(string username);
        Task RemoveConsumerForUserAsync(string username);
    }
}