using MongoDB.Bson.Serialization.Attributes;

namespace IdentityServer.Models
{
    public class UserMongoDocument
    {
        [BsonId]
        public string Username { get; set; }

        [BsonElement("hash")]
        public string PasswordHash { get; set; }

        [BsonElement("log_exchange")]
        public string LogExchangeName { get; set; }

        [BsonElement("feedback_queue")]
        public string FeedbackQueueName { get; set; }

        public static UserMongoDocument Create(User user)
        {
            if (user == null)
                return null;

            return new UserMongoDocument
            {
                Username = user.Username,
                PasswordHash = user.PasswordHash,
                LogExchangeName = user.LogExchangeName,
                FeedbackQueueName = user.FeedbackQueueName
            };
        }

        public User ToUser()
        {
            return new User
            {
                Username = Username,
                PasswordHash = PasswordHash,
                LogExchangeName = LogExchangeName,
                FeedbackQueueName = FeedbackQueueName
            };
        }
    }
}