using Newtonsoft.Json;

namespace Deer.Models.Elasticsearch
{
    public class CreateUserRequestModel
    {
        [JsonProperty("password")]
        public string Password { get; set; }
        
        [JsonProperty("backend_roles")]
        public string[] BackendRoles { get; set; }
    }
}