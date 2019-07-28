namespace IdentityServer.Models
{
    public class User
    {
        public string Username { get; set; }

        public string PasswordHash { get; set; }

        public string LogExchangeName { get; set; }

        public string FeedbackQueueName { get; set; }
    }
}