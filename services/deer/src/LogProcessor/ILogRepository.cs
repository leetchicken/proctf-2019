using System.Threading.Tasks;
using Deer.Messages;

namespace LogProcessor
{
    public interface ILogRepository
    {
        Task IndexAsync(string indexName, LogData logData);
    }
}