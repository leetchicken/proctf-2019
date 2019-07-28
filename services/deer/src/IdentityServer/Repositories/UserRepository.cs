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

        public async Task CreateAsync(User user)
        {
            await _userCollection.InsertOneAsync(UserMongoDocument.Create(user));
        }
    }
}