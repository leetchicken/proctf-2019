using System;
using System.Threading.Tasks;
using Microsoft.Extensions.Caching.Memory;

namespace notepool
{
	public static class Utils
	{
		public static async Task<TItem> GetOrCreateNotNullAsync<TItem>(this IMemoryCache cache, string key, Func<string, Task<TItem>> factory, Action<ICacheEntry> change)
		{
			if(!cache.TryGetValue(key, out object result))
			{
				result = await factory(key);
				if(result == null)
					return default;

				var entry = cache.CreateEntry(key);
				change(entry);
				entry.SetValue(result);
				entry.Dispose();
			}

			return (TItem)result;
		}

		public static int GetDeterministicHashCode(this string str)
		{
			unchecked
			{
				int hash1 = (5381 << 16) + 5381;
				int hash2 = hash1;

				for(int i = 0; i < str.Length; i += 2)
				{
					hash1 = ((hash1 << 5) + hash1) ^ str[i];
					if(i == str.Length - 1)
						break;
					hash2 = ((hash2 << 5) + hash2) ^ str[i + 1];
				}

				return hash1 + hash2 * 1566083941;
			}
		}

		public static string TrimToNull(this string value)
			=> string.IsNullOrEmpty(value = value?.Trim()) ? null : value;

		public static long? TryParseOrDefault(this string value)
			=> long.TryParse(value, out var gen) ? gen : (long?)null;

		public static long ToUnixTimeMinutes(this DateTime time)
			=> (time.Ticks - UnixTimeOffset) / 600000000L;

		public static DateTime FromUnixTimeMinutes(this long time)
			=> new DateTime(time * 600000000L + UnixTimeOffset, DateTimeKind.Utc);

		private static readonly long UnixTimeOffset = new DateTime(1970, 1, 1).Ticks;
	}
}
