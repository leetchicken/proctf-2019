using System;
using System.Text.Json.Serialization;

namespace checker.rubik
{
	internal class GeneratedRubik
	{
		[JsonPropertyName("rubik")]
		public string Rubik { get; set; }

		[JsonPropertyName("value")]
		public string Value { get; set; }
	}

	public class User
	{
		[JsonPropertyName("login")]
		public string Login { get; set; }
		[JsonPropertyName("bio")]
		public string Bio { get; set; }
		[JsonPropertyName("created")]
		public DateTime Created { get; set; }
	}

	public class Solution
	{
		[JsonPropertyName("id")]
		public Guid Id { get; set; }
		[JsonPropertyName("login")]
		public string Login { get; set; }
		[JsonPropertyName("movesCount")]
		public int MovesCount { get; set; }
		[JsonPropertyName("time")]
		public ulong Time { get; set; }
		[JsonPropertyName("created")]
		public DateTime Created { get; set; }
	}
}
