using Newtonsoft.Json;

namespace Deer.Models.Elasticsearch
{
    public class ElasticsearchResponseBaseModel
    {
        public ElasticsearchResponseBaseModel(string status, string message)
        {
            Status = status;
            Message = message;
        }

        [JsonProperty("status")]
        public string Status { get; private set; }
        
        [JsonProperty("message")]
        public string Message { get; private set; }
    }
}