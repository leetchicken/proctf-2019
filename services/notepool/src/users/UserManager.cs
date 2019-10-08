using System;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Extensions.Caching.Memory;

namespace notepool.users
{
	public class UserManager
	{
		public UserManager(IMemoryCache cache)
		{
			this.cache = cache;
			Directory.CreateDirectory(UsersDirectoryPath);
		}

		public Task<User> FindAsync(string login) => login == null ? null : cache.GetOrCreateNotNullAsync(login.ToLower(), async entry =>
		{
			var path = GetFilePath(entry);
			if(!File.Exists(path))
				return null;

			await using var reader = new UserReaderWriter(path);
			return await reader.ReadUserAsync();
		}, entry => entry.SetSlidingExpiration(CacheRecordTtl));

		public async Task AddAsync(User user)
		{
			await using var writer = new UserReaderWriter(GetFilePath(user.Login.ToLower()));
			await writer.WriteUserAsync(user);
		}

		private static string GetFilePath(string name)
			=> Path.Combine(UsersDirectoryPath, (name.GetDeterministicHashCode() & 0xfff).ToString("x3"), name);

		private const string UsersDirectoryPath = "data/users";
		private static readonly TimeSpan CacheRecordTtl = TimeSpan.FromMinutes(1);
		private readonly IMemoryCache cache;
	}

	public class UserReaderWriter : IAsyncDisposable
	{
		public UserReaderWriter(string path)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(path));
			stream = new FileStream(path, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite, BufferSize, FileOptions.Asynchronous);
		}

		public async Task<User> ReadUserAsync()
		{
			stream.Seek(0, SeekOrigin.Begin);
			using var reader = new StreamReader(stream, Encoding.UTF8, false, BufferSize, true);
			return new User
			{
				Key = Guid.Parse(await reader.ReadLineAsync()).ToByteArray(),
				Password = await reader.ReadLineAsync(),
				Name = await reader.ReadLineAsync(),
				Login = await reader.ReadLineAsync()
			};
		}

		public async Task WriteUserAsync(User user)
		{
			await using var writer = new StreamWriter(stream, Encoding.UTF8, BufferSize, true) {NewLine = "\n"};
			await writer.WriteLineAsync(new Guid(user.Key).ToString());
			await writer.WriteLineAsync(user.Password);
			await writer.WriteLineAsync(user.Name);
			await writer.WriteAsync(user.Login);
		}

		public ValueTask DisposeAsync() => stream.DisposeAsync();

		private const int BufferSize = 1024;
		private readonly Stream stream;
	}
}
