using System.Threading.Tasks;
using Deer.Messages;

namespace Deer.Repositories
{
    public interface ILogRepository
    {
        Task IndexAsync(string indexName, LogData logData);
    }
}