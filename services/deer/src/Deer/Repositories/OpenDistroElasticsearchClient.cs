using System;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.Mime;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;
using Deer.Models.Elasticsearch;
using Elasticsearch.Net;
using Microsoft.Extensions.Logging;
using Nest;
using Newtonsoft.Json;

namespace Deer.Repositories
{
    public class OpenDistroElasticsearchClient : IOpenDistroElasticsearchClient, IDisposable
    {
        private readonly ILogger<OpenDistroElasticsearchClient> _logger;
        private readonly HttpClient _httpClient;
        private readonly ElasticClient _elasticClient;
        private readonly JsonSerializerSettings _jsonSerializerSettings = new JsonSerializerSettings();

        public OpenDistroElasticsearchClient(Uri elasticSearchUri, ILogger<OpenDistroElasticsearchClient> logger)
        {
            _logger = logger;
            var userInfo = elasticSearchUri.UserInfo.Split(':');
            var user = userInfo[0];
            var password = userInfo[1];

            var rootCaCert = new X509Certificate2("root-ca.pem");
            
            var httpClientHandler = new HttpClientHandler
            {
                ServerCertificateCustomValidationCallback = (message, cert, chain, errors) =>
                {
                    chain.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;
                    chain.ChainPolicy.ExtraStore.Add(rootCaCert);
                    chain.ChainPolicy.VerificationFlags = X509VerificationFlags.AllowUnknownCertificateAuthority;
                    return chain.Build(cert);
                }
            };

            var authHeader = new AuthenticationHeaderValue("Basic", Convert.ToBase64String(Encoding.UTF8.GetBytes($"{user}:{password}")));

            _httpClient = new HttpClient(httpClientHandler, true) {BaseAddress = elasticSearchUri};
            _httpClient.DefaultRequestHeaders.Authorization = authHeader;

            var nestSettings = new ConnectionSettings(elasticSearchUri)
                .BasicAuthentication(user, password)
                .ServerCertificateValidationCallback(CertificateValidations.AuthorityIsRoot(rootCaCert));
            
            _elasticClient = new ElasticClient(nestSettings);
        }

        public async Task CreateUserAsync(string username, string password)
        {
            var createUserJson = JsonConvert.SerializeObject(new CreateUserRequestModel(password, "deer_user"));
            var content = new StringContent(createUserJson, Encoding.UTF8, MediaTypeNames.Application.Json);
            var httpResponse = await _httpClient.PutAsync($"/_opendistro/_security/api/internalusers/{username}", content);
            httpResponse.EnsureSuccessStatusCode();
            var response = new SafeJsonDeserializer<ElasticsearchResponseBaseModel>(await httpResponse.Content.ReadAsStringAsync(), _jsonSerializerSettings);
            if (response.Model.Status != "CREATED")
                _logger.LogWarning($"Create user response status for user '{username}' is unknown: {response.Model.Status}");
            else
                _logger.LogInformation($"Elasticsearch response: {response.Model.Message}");
        }

        public async Task DeleteUserAsync(string username)
        {
            var httpResponse = await _httpClient.DeleteAsync($"/_opendistro/_security/api/internalusers/{username}");
            httpResponse.EnsureSuccessStatusCode();
            var response = new SafeJsonDeserializer<ElasticsearchResponseBaseModel>(await httpResponse.Content.ReadAsStringAsync(), _jsonSerializerSettings);
            if (response.Model.Status != "OK")
                _logger.LogWarning($"Delete user response status for user '{username}' is unknown: {response.Model.Status}");
            else
                _logger.LogInformation($"Elasticsearch response: {response.Model.Message}");
        }

        public async Task CreateIndexAsync(string index)
        {
            await _elasticClient.Indices.CreateAsync(index, cd => cd.Settings(isd => isd.NumberOfShards(1).NumberOfReplicas(0)));
        }

        public async Task DeleteIndexAsync(string index)
        {
            await _elasticClient.Indices.DeleteAsync(index);
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
        }
    }
}