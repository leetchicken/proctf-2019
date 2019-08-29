using System;
using Newtonsoft.Json;

namespace Deer.Models.Elasticsearch
{
    // TODO use
    public class SafeJsonDeserializer<T>
    {
        public SafeJsonDeserializer(string json, JsonSerializerSettings settings)
        {
            try
            {
                Instance = JsonConvert.DeserializeObject<T>(json, settings);
            }
            catch (Exception)
            {
            }
        }

        public T Instance { get; }
    }
}