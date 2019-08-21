using System.Threading.Tasks;

namespace LogProcessor
{
    public interface ILogConsumerService
    {
        Task StartAsync();
        Task StopAsync();
        Task AddConsumerForUserAsync(string username);
        Task RemoveConsumerForUserAsync(string username);
    }
}