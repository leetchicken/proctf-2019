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
using rubiksolver;

namespace checker.rubik
{
	internal class RubikChecker : IChecker
	{
		public Task<string> Info() => Task.FromResult("vulns: 1");

		public async Task Check(string host)
		{
			var client = new AsyncHttpClient(GetBaseUri(host), true);

			var result = await client.DoRequestAsync(HttpMethod.Get, "/", null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), "get / failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			result = await client.DoRequestAsync(HttpMethod.Get, ApiGenerate).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiGenerate} failed");

			var rubik = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<GeneratedRubik>(result.BodyAsString));
			if(rubik == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiGenerate} response");

			await Console.Error.WriteLineAsync($"rubik '{rubik.Rubik}', signed '{rubik.Value}'").ConfigureAwait(false);

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var login = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();
			var pass = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();
			var flag = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();

			await Console.Error.WriteLineAsync($"name '{login}', pass '{pass}', bio '{flag}'").ConfigureAwait(false);

			var solution = RndRubik.RandomSolution(MinRandomSolutionLength, MaxRandomSolutionLength);
			var query = $"?login={WebUtility.UrlEncode(login)}&pass={WebUtility.UrlEncode(pass)}&bio={WebUtility.UrlEncode(flag)}&puzzle={WebUtility.UrlEncode(rubik.Value)}&solution={WebUtility.UrlEncode(solution)}";

			result = await client.DoRequestAsync(HttpMethod.Post, ApiSolve + query, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != (HttpStatusCode)418)
				throw new CheckerException(result.StatusCode == HttpStatusCode.OK ? ExitCode.MUMBLE : result.StatusCode.ToExitCode(), $"post {ApiSolve} failed");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			result = await client.DoRequestAsync(HttpMethod.Get, ApiAuth).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiAuth} failed");

			var user = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<User>(result.BodyAsString));
			if(user == default || user.Login != login || user.Bio != flag)
				throw new CheckerException(ExitCode.MUMBLE, "invalid {ApiAuth} response");
		}

		public async Task<string> Put(string host, string id, string flag, int vuln)
		{
			var client = new AsyncHttpClient(GetBaseUri(host), true);

			var result = await client.DoRequestAsync(HttpMethod.Get, ApiGenerate).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiGenerate} failed");

			var rubik = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<GeneratedRubik>(result.BodyAsString));
			if(rubik == default)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiGenerate} response");

			await Console.Error.WriteLineAsync($"rubik '{rubik.Rubik}', signed '{rubik.Value}'").ConfigureAwait(false);

			string solution;
			try
			{
				solution = DoIt.TryOrDefault(() => SolveHelper.ConvertOutputSolution(RubikSolver.FindSolution(SolveHelper.ConvertInputCube(rubik.Rubik), 32, 10000)));
			}
			catch(RubikSolveException e)
			{
				await Console.Error.WriteLineAsync(e.Message).ConfigureAwait(false);
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiGenerate} response");
			}

			await Console.Error.WriteLineAsync($"solution '{solution}'").ConfigureAwait(false);

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			var login = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();
			var pass = RndText.RandomWord(RndUtil.GetInt(MinRandomFieldLength, MaxRandomFieldLength)).RandomLeet().RandomUpperCase();

			await Console.Error.WriteLineAsync($"name '{login}', pass '{pass}', bio '{flag}'").ConfigureAwait(false);

			var query = $"?login={WebUtility.UrlEncode(login)}&pass={WebUtility.UrlEncode(pass)}&bio={WebUtility.UrlEncode(flag)}&puzzle={WebUtility.UrlEncode(rubik.Value)}&solution={WebUtility.UrlEncode(solution)}";

			result = await client.DoRequestAsync(HttpMethod.Post, ApiSolve + query, null, NetworkOpTimeout, MaxHttpBodySize).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"post {ApiSolve} failed");

			var sln = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<Solution>(result.BodyAsString));
			if(sln == default || sln.Login != login || sln.MovesCount != solution.Length)
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiGenerate} response");

			await Console.Error.WriteLineAsync($"solution '{solution}'").ConfigureAwait(false);

			var cookie = DoIt.TryOrDefault(() => WebUtility.UrlDecode(client.Cookies?.GetCookies(GetBaseUri(host))[AuthCookieName]?.Value));
			if(string.IsNullOrEmpty(cookie) || cookie.Length > 1024)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"invalid {ApiSolve} response");

			return $"{login}:{pass}:{Convert.ToBase64String(Encoding.UTF8.GetBytes(cookie))}";
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

			var result = await client.DoRequestAsync(HttpMethod.Get, ApiScoreboard).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiScoreboard} failed");

			var solutions = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<List<Solution>>(result.BodyAsString));
			if(solutions == default || solutions.Count == 0 || solutions.All(sln => sln.Login != login))
				throw new CheckerException(ExitCode.MUMBLE, $"invalid {ApiGenerate} response");

			await RndUtil.RndDelay(MaxDelay).ConfigureAwait(false);

			client = new AsyncHttpClient(GetBaseUri(host), true);
			client.Cookies.Add(GetBaseUri(host), new Cookie(AuthCookieName, cookie, "/"));

			result = await client.DoRequestAsync(HttpMethod.Get, ApiAuth).ConfigureAwait(false);
			if(result.StatusCode != HttpStatusCode.OK)
				throw new CheckerException(result.StatusCode.ToExitCode(), $"get {ApiAuth} failed");

			var user = DoIt.TryOrDefault(() => JsonSerializer.Deserialize<User>(result.BodyAsString));
			if(user == default || user.Login != login || user.Bio != flag)
				throw new CheckerException(ExitCode.CORRUPT, "flag not found");
		}

		private const int Port = 5071;

		private const int MaxHttpBodySize = 16 * 1024;
		private const int NetworkOpTimeout = 5000;

		private const int MaxDelay = 1000;

		private const int MinRandomSolutionLength = 5;
		private const int MaxRandomSolutionLength = 500;

		private const int MinRandomFieldLength = 10;
		private const int MaxRandomFieldLength = 32;

		private static Uri GetBaseUri(string host) => new Uri($"https://{host}:{Port}/");

		private const string ApiScoreboard = "/api/scoreboard";
		private const string ApiGenerate = "/api/generate";
		private const string ApiSolve = "/api/solve";
		private const string ApiAuth = "/api/auth";

		private const string AuthCookieName = "AUTH";
	}
}
