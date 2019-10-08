using System;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

namespace notepool
{
	public static class TextHash
	{
		public static string HashWord(this HMACMD5 hmac, string word)
		{
			var bytes = Encoding.UTF8.GetBytes(word);
			return new Guid(hmac.ComputeHash(bytes, 0, bytes.Length)).ToString("N");
		}

		public static string HashWord(this HMACMD5 hmac, char[] word, int offset, int count)
		{
			var bytes = Encoding.UTF8.GetBytes(word, offset, count);
			return new Guid(hmac.ComputeHash(bytes, 0, bytes.Length)).ToString("N");
		}

		public static string HashWords(this string text, byte[] key)
		{
			if(text == null || key == null)
				return text;

			using var hmac = new HMACMD5(key);
			return WordRegex.Replace(text, match => hmac.HashWord(match.Value.ToLower()));
		}

		private static readonly Regex WordRegex = new Regex(@"\w+", RegexOptions.Compiled);
	}
}
