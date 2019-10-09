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

			var query = RndText.RandomWord(RndUtil.GetInt(2, 10));
			result = await client.DoRequestAsync(HttpMethod.Get, ApiNotesSearch + "?query=" + WebUtility.UrlEncode(query), null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
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

			var login = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet()/*.RandomUpperCase()*/;
			var name = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();
			var pass = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();

			await Console.Error.WriteLineAsync($"login '{login}', name '{name}', pass '{pass}'").ConfigureAwait(false);

			var signUpQuery = $"?login={WebUtility.UrlEncode(login)}&name={WebUtility.UrlEncode(name)}&password={WebUtility.UrlEncode(pass)}";
			var result = await client.DoRequestAsync(HttpMethod.Post, ApiAuthSignUp + signUpQuery, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiAuthSignUp} failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var title1 = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength));
			var text1 = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength));
			await Console.Error.WriteLineAsync($"title '{title1}', text '{text1}'").ConfigureAwait(false);

			var query1 = $"?title={WebUtility.UrlEncode(title1)}&text={WebUtility.UrlEncode(text1)}";
			result = await client.DoRequestAsync(HttpMethod.Post, ApiNotesAdd + query1, null, NetworkOpTimeout).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"post {ApiNotesAdd} failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var title2 = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength));
			var text2 = flag;
			await Console.Error.WriteLineAsync($"title '{title2}', text '{text2}'").ConfigureAwait(false);

			var query2 = $"?title={WebUtility.UrlEncode(title2)}&text={WebUtility.UrlEncode(text2)}&isPrivate=on";
			result = await client.DoRequestAsync(HttpMethod.Post, ApiNotesAdd + query2, null, NetworkOpTimeout).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"post {ApiNotesAdd} failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var query = flag;
			result = await client.DoRequestAsync(HttpMethod.Get, ApiNotesSearch + "?query=" + query, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiNotesSearch} failed");

			var notes = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<List<Note>>(result.BodyAsString));
			if(notes == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response");

			await Console.Error.WriteLineAsync($"found '{notes.Count}' notes by query '{query}'").ConfigureAwait(false);

			if(notes.Count > 1)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response");

			if(notes.Count == 0 || !notes.Any(note => note.Author.Contains(flag) || note.Title.Contains(flag) || note.Text.Contains(flag)))
				throw new CheckerException(ExitCode.CORRUPT, "flag not found");

			var cookie = client.Cookies.GetCookieHeader(GetBaseUri(host));
			await Console.Error.WriteLineAsync($"cookie '{cookie}'").ConfigureAwait(false);

			var bytes = DoIt.TryOrDefault(() => Encoding.UTF8.GetBytes(cookie));
			if(bytes == null || bytes.Length > 1024)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"invalid {ApiNotesSearch} response: cookies");

			return $"{login}:{pass}:{Convert.ToBase64String(bytes)}";
		}

		public async Task Get(string host, string id, string flag, int vuln)
		{
			var parts = id.Split(':');
			if(parts.Length != 3)
				throw new Exception($"Invalid flag id '{id}'");

			var login = parts[0];
			var pass = parts[1];
			var cookie = Encoding.UTF8.GetString(Convert.FromBase64String(parts[2]));

			var client = new AsyncHttpClient(GetBaseUri(host), true);

			if(RndUtil.Choice(1, 2) == 1)
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

			var result = await client.DoRequestAsync(HttpMethod.Get, ApiNotesSearch + "?query=&myOnly=on", null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiNotesSearch} failed");

			var notes = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<List<Note>>(result.BodyAsString));
			if(notes == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiNotesSearch} response");

			await Console.Error.WriteLineAsync($"found '{notes.Count}' myOnly notes").ConfigureAwait(false);

			if(notes.Count == 0 || !notes.Any(note => note.Author.Contains(flag) || note.Title.Contains(flag) || note.Text.Contains(flag)))
				throw new CheckerException(ExitCode.CORRUPT, "flag not found");
		}

		private const int Port = 5073;

		private const int MaxHttpBodySize = 1024 * 1024;
		private const int NetworkOpTimeout = 10000;

		private const int MaxDelay = 1000;

		private const int MinRandomFieldLength = 10;
		private const int MaxRandomFieldLength = 16;

		private static Uri GetBaseUri(string host) => new Uri($"http://{host}:{Port}/");

		private const string ApiAuthSignUp = "/auth/signup";
		private const string ApiAuthSignIn = "/auth/signin";
		private const string ApiNotesAdd = "/notes/add";
		private const string ApiNotesSearch = "/notes/search";

		private const string AuthCookieName = "usr";
		private const string GenCookieName = "usr";
	}
}
