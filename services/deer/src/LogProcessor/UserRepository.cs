using System.Collections.Generic;
using System.Threading.Tasks;
using LogProcessor.Models;
using MongoDB.Driver;

namespace LogProcessor
{
    public class UserRepository : IUserRepository
    {
        private readonly IMongoCollection<UserMongoDocument> _userCollection;

        public UserRepository(IMongoCollection<UserMongoDocument> userCollection)
        {
            _userCollection = userCollection;
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
    }
}