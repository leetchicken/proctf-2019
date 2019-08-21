using System;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.Mime;
using System.Text;
using System.Threading.Tasks;
using Elasticsearch.Net;
using IdentityServer.Models.Elasticsearch;
using Nest;
using Newtonsoft.Json;

namespace IdentityServer.Repositories
{
    public class OpenDistroElasticsearchClient : IOpenDistroElasticsearchClient, IDisposable
    {
        private readonly HttpClient _httpClient;
        private readonly ElasticClient _elasticClient;

        public OpenDistroElasticsearchClient(Uri elasticSearchUri)
        {
            var userInfo = elasticSearchUri.UserInfo.Split(':');
            var user = userInfo[0];
            var password = userInfo[1];
            ServicePointManager.ServerCertificateValidationCallback +=
                (sender, cert, chain, errors) => true;
            
            // TODO remove
            var httpClientHandler = new HttpClientHandler
            {
                ServerCertificateCustomValidationCallback = (message, cert, chain, errors) => { return true; }
            };

            var authHeader = new AuthenticationHeaderValue("Basic", Convert.ToBase64String(Encoding.UTF8.GetBytes($"{user}:{password}")));

            _httpClient = new HttpClient(httpClientHandler, true) {BaseAddress = elasticSearchUri};
            _httpClient.DefaultRequestHeaders.Authorization = authHeader;
            
            var nestSettings = new ConnectionSettings(elasticSearchUri)
            .BasicAuthentication(user, password)
            // TODO change to AuthorityIsRoot
            .ServerCertificateValidationCallback(CertificateValidations.AllowAll);
            
            _elasticClient = new ElasticClient(nestSettings);
        }

        public async Task CreateUserAsync(string username, string password)
        {
            var createUserJson = JsonConvert.SerializeObject(new CreateUserRequestModel {Password = password, BackendRoles = new[] {"deer_user"}});
            var content = new StringContent(createUserJson, Encoding.UTF8, MediaTypeNames.Application.Json);
            var response = await _httpClient.PutAsync($"/_opendistro/_security/api/internalusers/{username}", content);
            response.EnsureSuccessStatusCode();
        }

        public async Task CreateIndexAsync(string index)
        {
            await _elasticClient.Indices.CreateAsync(index, cd => cd.Settings(isd => isd.NumberOfShards(1).NumberOfReplicas(0)));
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
        }
    }
}