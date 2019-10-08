using System;
using System.Text.Json.Serialization;

namespace notepool
{
	public class Note
	{
		[JsonPropertyName("author")] public string Author { get; set; }
		[JsonPropertyName("title")] public string Title { get; set; }
		[JsonPropertyName("text")] public string Text { get; set; }
		[JsonPropertyName("time")] public DateTime Time { get; set; }
	}
}
