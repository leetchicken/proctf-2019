using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using checker.net;
using checker.rnd;
using checker.utils;

namespace checker.notepool
{
	internal class NotepoolChecker : IChecker
	{
		public Task<string> Info() => Task.FromResult("vulns: 1");

		public async Task Check(string host)
		{
			var client = new AsyncHttpClient(GetBaseUri(host), true);

			var result = await client.DoRequestAsync(HttpMethod.Get, "/", null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), "get / failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var query = "?query=" + WebUtility.UrlEncode(RndText.RandomWord(RndUtil.GetInt(2, 10)));
			result = await client.DoRequestAsync(HttpMethod.Get, ApiNotesSearch + query, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiNotesSearch} failed");

			var notes = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<List<Note>>(result.BodyAsString));
			if(notes == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response");

			await Console.Error.WriteLineAsync($"found '{notes.Count}' notes by query '{query}'").ConfigureAwait(false);
		}

		public async Task<string> Put(string host, string id, string flag, int vuln)
		{
			var client = new AsyncHttpClient(GetBaseUri(host), true);

			var login = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();
			var name = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();
			var pass = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();

			await Console.Error.WriteLineAsync($"login '{login}', name '{name}', pass '{pass}'").ConfigureAwait(false);

			var signUpQuery = $"?login={WebUtility.UrlEncode(login)}&name={WebUtility.UrlEncode(name)}&password={WebUtility.UrlEncode(pass)}";
			var result = await client.DoRequestAsync(HttpMethod.Post, ApiAuthSignUp + signUpQuery, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiAuthSignUp} failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var items = Enumerable.Range(0, RndUtil.GetInt(1, 3)).Select(i => new Note {
				Title = RndText.RandomText(RndUtil.GetInt(MinRandomTitleLength, MaxRandomTitleLength)).RandomUmlauts(),
				Text = RndText.RandomText(RndUtil.GetInt(MinRandomTextLength, MaxRandomTextLength)).RandomUmlauts()
			}).ToArray();

			var itemWithFlag = RndUtil.Choice(items);
			if(RndUtil.GetInt(0, 2) == 0)
				itemWithFlag.Text = flag;
			else
				itemWithFlag.Title = flag;
			itemWithFlag.IsPrivate = true;

			foreach(var item in items)
			{
				await Console.Error.WriteLineAsync($"title '{item.Title}', text '{item.Text}', isPrivate '{item.IsPrivate}'").ConfigureAwait(false);

				var q = $"?title={WebUtility.UrlEncode(item.Title)}&text={WebUtility.UrlEncode(item.Text)}" + (item.IsPrivate ? "&isPrivate=on" : null);
				result = await client.DoRequestAsync(HttpMethod.Post, ApiNotesAdd + q, null, NetworkOpTimeout).ConfigureAwait(false);
				if(result.StatusCode != HttpStatusCode.OK)
					throw new CheckerException(result.StatusCode.ToExitCode(), $"post {ApiNotesAdd} failed");

				await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);
			}

			var query = GetRandomQuery(itemWithFlag);
			if(query.Trim('\"', ' ').Length <= 4) //NOTE: too low entropy
				query = flag;

			await Console.Error.WriteLineAsync($"random query '{query}'").ConfigureAwait(false);

			var cookie = client.Cookies.GetCookieHeader(GetBaseUri(host));
			await Console.Error.WriteLineAsync($"cookie '{cookie}'").ConfigureAwait(false);

			var bytes = DoIt.TryOrDefault(() => Encoding.UTF8.GetBytes(cookie));
			if(bytes == null || bytes.Length > 1024)
				throw new CheckerException(result.StatusCode.ToExitCode(), "too large or invalid cookies");

			return $"{login}:{pass}:{Convert.ToBase64String(bytes)}:{WebUtility.UrlEncode(query)}";
		}

		public async Task Get(string host, string id, string flag, int vuln)
		{
			var parts = id.Split(':', 4);
			if(parts.Length != 4)
				throw new Exception($"Invalid flag id '{id}'");

			var login = parts[0];
			var pass = parts[1];
			var cookie = Encoding.UTF8.GetString(Convert.FromBase64String(parts[2]));
			var encodedQuery = parts[3];

			var client = new AsyncHttpClient(GetBaseUri(host), true);

			if(RndUtil.GetInt(0, 2) == 0)
			{
				await Console.Error.WriteLineAsync($"login by cookie '{cookie}'").ConfigureAwait(false);
				client.Cookies.SetCookies(GetBaseUri(host), cookie);
			}
			else
			{
				await Console.Error.WriteLineAsync($"login with login '{login}' and pass '{pass}'").ConfigureAwait(false);

				var loginQuery = $"?login={WebUtility.UrlEncode(login)}&password={WebUtility.UrlEncode(pass)}";
				var signInResult = await client.DoRequestAsync(HttpMethod.Post, ApiAuthSignIn + loginQuery).ConfigureAwait(false);
				if(signInResult.StatusCode != HttpStatusCode.OK)
					throw new CheckerException(signInResult.StatusCode.ToExitCode(), $"get {ApiAuthSignIn} failed");

				await Console.Error.WriteLineAsync($"cookie '{client.Cookies.GetCookieHeader(GetBaseUri(host))}'").ConfigureAwait(false);

				await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);
			}

			var query = RndUtil.Choice("?query=&myOnly=on", "?query=" + encodedQuery);

			var result = await client.DoRequestAsync(HttpMethod.Get, ApiNotesSearch + query, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiNotesSearch} failed");

			var notes = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<List<Note>>(result.BodyAsString));
			if(notes == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response");

			await Console.Error.WriteLineAsync($"found '{notes.Count}' notes by query '{query}'").ConfigureAwait(false);

			var note = notes.FirstOrDefault(note => note.Author.Contains(flag) || note.Title.Contains(flag) || note.Text.Contains(flag));
			if(note == null)
				throw new CheckerException(ExitCode.CORRUPT, "flag not found");

			//NOTE: Also check phrase query

			if(!query.StartsWith("?query=%22") || !query.EndsWith("%22"))
				return;

			var words = WebUtility.UrlDecode(encodedQuery).Trim('"', ' ').Split().Where(word => !string.IsNullOrWhiteSpace(word)).Distinct().ToArray();
			if(words.Length < 2)
				return;

			query = string.Join(' ', words.Reverse());
			if(note.Author.Contains(query, StringComparison.InvariantCultureIgnoreCase) || note.Title.Contains(query, StringComparison.InvariantCultureIgnoreCase) || note.Text.Contains(query, StringComparison.InvariantCultureIgnoreCase))
				return;

			query = "?query=" + WebUtility.UrlEncode('"' + query + '"');
			await Console.Error.WriteLineAsync($"check phrase query reversed '{query}'").ConfigureAwait(false);

			result = await client.DoRequestAsync(HttpMethod.Get, ApiNotesSearch + query, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiNotesSearch} failed");

			notes = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<List<Note>>(result.BodyAsString));
			if(notes == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response");

			await Console.Error.WriteLineAsync($"found '{notes.Count}' notes by query '{query}'").ConfigureAwait(false);

			if(notes.Any(note => note.Author.Contains(flag) || note.Title.Contains(flag) || note.Text.Contains(flag)))
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response: phrase query");
		}

		private string GetRandomQuery(Note note)
		{
			var titleWords = note.Title.Split();
			var textWords = note.Text.Split();

			var allWords = titleWords.Concat(textWords).ToArray();
			var randomQuery = string.Join(' ', Enumerable.Range(0, RndUtil.GetInt(2, 4)).Select(i => RndUtil.Choice(allWords)).Distinct());

			var words = RndUtil.Choice(note.Title, note.Text).Split();
			var skip = RndUtil.GetInt(0, Math.Max(0, words.Length - 2));
			var randomPhraseQuery = '\"' + string.Join(' ', words.Skip(skip).Take(RndUtil.GetInt(2, 6))) + '\"';

			return RndUtil.Choice(randomQuery, randomPhraseQuery);
		}

		private const int Port = 5073;

		private const int MaxHttpBodySize = 1024 * 1024;
		private const int NetworkOpTimeout = 10000;

		private const int MaxDelay = 1000;

		private const int MinRandomFieldLength = 10;
		private const int MaxRandomFieldLength = 16;
		private const int MinRandomTitleLength = 5;
		private const int MaxRandomTitleLength = 64;
		private const int MinRandomTextLength = 16;
		private const int MaxRandomTextLength = 256;

		private static Uri GetBaseUri(string host) => new Uri($"http://{host}:{Port}/");

		private const string ApiAuthSignUp = "/auth/signup";
		private const string ApiAuthSignIn = "/auth/signin";
		private const string ApiNotesAdd = "/notes/add";
		private const string ApiNotesSearch = "/notes/search";
	}
}
