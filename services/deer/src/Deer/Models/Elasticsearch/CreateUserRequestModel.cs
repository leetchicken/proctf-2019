using Newtonsoft.Json;

namespace Deer.Models.Elasticsearch
{
    public class CreateUserRequestModel
    {
        public CreateUserRequestModel(string password, params string[] backendRoles)
        {
            Password = password;
            BackendRoles = backendRoles;
        }

        [JsonProperty("password")]
        public string Password { get; private set; }
        
        [JsonProperty("backend_roles")]
        public string[] BackendRoles { get; private set; }
    }
}