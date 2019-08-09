using System.Threading.Tasks;
using IdentityServer.Models;
using MongoDB.Driver;

namespace IdentityServer.Repositories
{
    public class UserRepository : IUserRepository
    {
        private readonly IMongoCollection<UserMongoDocument> _userCollection;

        public UserRepository(IMongoCollection<UserMongoDocument> userCollection)
        {
            _userCollection = userCollection;
        }

        public async Task<User> GetAsync(string username)
        {
            var filter = Builders<UserMongoDocument>.Filter.Eq(d => d.Username, username);
            var doc = (await _userCollection.FindAsync(filter, new FindOptions<UserMongoDocument> {Limit = 1})).SingleOrDefault();
            return doc?.ToUser();
        }

        public async Task CreateAsync(User user)
        {
            await _userCollection.InsertOneAsync(UserMongoDocument.Create(user));
        }
    }
}