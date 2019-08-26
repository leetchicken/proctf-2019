// ReSharper disable once CheckNamespace
namespace Deer.Messages
{
    public class Error
    {
        public string ErrorMessage { get; set; }
        
        public byte[] RawMessage { get; set; }
    }
}