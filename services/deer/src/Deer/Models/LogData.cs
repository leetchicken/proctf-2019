using Nest;
using Newtonsoft.Json;

// ReSharper disable once CheckNamespace
namespace Deer.Messages
{
    [ElasticsearchType(RelationName = "logs")]
    public class LogData
    {
        [JsonProperty("content")]
        public string Content { get; set; }
    }
}