namespace LogProcessor.Models
{
    public class UserAddedMessage
    {
        public string Username { get; set; }

        public string LogExchangeName { get; set; }

        public string FeedbackQueueName { get; set; }
    }
}