using System;
using System.Security.Cryptography;

namespace rubikxplt
{
	public static class TokenCrypt
	{
		public static void Encrypt(ReadOnlySpan<byte> key, ReadOnlySpan<byte> plain, ref Span<char> token)
		{
			Span<byte> data = stackalloc byte[TagSize + NonceSize + plain.Length];
			var tag = data.Slice(0, TagSize);
			var nonce = data.Slice(TagSize, NonceSize);
			var cipher = data.Slice(TagSize + NonceSize);

			RandomNumberGenerator.Fill(nonce);

			using var aes = new AesGcm(key);
			aes.Encrypt(nonce, plain, cipher, tag);

			if(!Convert.TryToBase64Chars(data, token, out var chars))
				throw new Exception("Token is too short");

			token = token.Slice(0, chars);
		}

		public static void Decrypt(ReadOnlySpan<byte> key, ReadOnlySpan<byte> data, ref Span<byte> plain)
		{
			var size = data.Length - TagSize - NonceSize;
			plain = plain.Slice(0, size);

			var tag = data.Slice(0, TagSize);
			var nonce = data.Slice(TagSize, NonceSize);
			var cipher = data.Slice(TagSize + NonceSize, size);

			using var aes = new AesGcm(key);
			aes.Decrypt(nonce, cipher, tag, plain);
		}

		private const int NonceSize = 12;
		private const int TagSize = 12;
	}
}
