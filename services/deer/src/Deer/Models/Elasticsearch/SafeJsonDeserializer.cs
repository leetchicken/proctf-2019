using System;
using Newtonsoft.Json;

namespace Deer.Models.Elasticsearch
{
    public class SafeJsonDeserializer<T>
    {
        public SafeJsonDeserializer(string json, JsonSerializerSettings settings)
        {
            try
            {
                Model = JsonConvert.DeserializeObject<T>(json, settings);
            }
            catch (Exception e)
            {
                Error = e;
            }
        }

        public T Model { get; }
        
        public Exception Error { get; }
    }
}