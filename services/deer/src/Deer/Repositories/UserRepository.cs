using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Deer.Models;
using Deer.Models.MongoDb;
using MongoDB.Driver;

namespace Deer.Repositories
{
    public class UserRepository : IUserRepository
    {
        private readonly IMongoCollection<UserMongoDocument> _userCollection;

        public UserRepository(IMongoCollection<UserMongoDocument> userCollection)
        {
            _userCollection = userCollection;
        }

        public async Task DeleteAsync(string username)
        {
            var filter = Builders<UserMongoDocument>.Filter.Eq(d => d.Username, username);
            await _userCollection.DeleteOneAsync(filter);
        }

        public async Task<IEnumerable<User>> GetUsersAsync()
        {
            var filter = Builders<UserMongoDocument>.Filter.Empty;
            return ToUsers((await _userCollection.FindAsync(filter)).ToEnumerable());
        }

        private static IEnumerable<User> ToUsers(IEnumerable<UserMongoDocument> docs)
        {
            foreach (var doc in docs)
            {
                yield return doc.ToUser();
            }
        }

        public async Task<User> GetUserAsync(string username)
        {
            var filter = Builders<UserMongoDocument>.Filter.Eq(d => d.Username, username);
            var doc = (await _userCollection.FindAsync(filter, new FindOptions<UserMongoDocument> {Limit = 1})).SingleOrDefault();
            return doc?.ToUser();
        }

        public User GetUserByLogQueueAsync(string logQueueName)
        {
            var filter = Builders<UserMongoDocument>.Filter.Eq(d => d.LogQueueName, logQueueName);
            var doc = _userCollection.FindSync(filter, new FindOptions<UserMongoDocument> {Limit = 1}).SingleOrDefault();
            return doc?.ToUser();
        }

        public async Task<IEnumerable<User>> GetOldUsersAsync()
        {
            var filter = Builders<UserMongoDocument>.Filter.Lt(d => d.Created, DateTime.UtcNow.AddMinutes(-1));
            return ToUsers((await _userCollection.FindAsync(filter)).ToEnumerable());
        }

        public async Task CreateAsync(User user)
        {
            await _userCollection.InsertOneAsync(UserMongoDocument.Create(user));
        }
    }
}