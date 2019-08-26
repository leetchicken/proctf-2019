using System.Reflection;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

namespace Deer.Models.Elasticsearch
{
    // TODO use
    public class ElasticResponse<T>
    {
        public ElasticResponse(string json)
        {
            var contractResolver = new DefaultContractResolver();
            contractResolver.DefaultMembersSearchFlags |= BindingFlags.NonPublic;

            var settings = new JsonSerializerSettings
            {
                TypeNameHandling = TypeNameHandling.Auto,
                ContractResolver = contractResolver
            };

            Instance = JsonConvert.DeserializeObject<T>(json, settings);
        }

        public T Instance { get; }
    }
}