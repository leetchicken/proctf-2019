using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Text;

namespace SPN
{
	public static class ByteArrayUtils
	{
		public static string ToHex(this byte[] bytes)
		{
			return bytes.ToHex(0, bytes.Length);
		}

		public static string ToHexUpperCase(this byte[] bytes)
		{
			return bytes.ToHex(0, bytes.Length).ToUpperInvariant();
		}

		public static string ToHex(this byte[] bytes, int offset, int count)
		{
			if(bytes.Length < offset + count)
				throw new ArgumentOutOfRangeException();
			var array = new char[count << 1];
			for(int i = 0; i < count; i++)
			{
				var b = bytes[offset + i];
				var hb = (byte)(b >> 4);
				array[i << 1] = ToHex4B(hb);
				var lb = (byte)(b & 0xf);
				array[(i << 1) + 1] = ToHex4B(lb);
			}
			return new string(array);
		}

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		public static char ToHex4B(this byte b)
		{
			return (char)(b > 9 ? b + ByteA - 10 : b + Byte0);
		}

		private const int Byte0 = '0';
		private const int ByteA = 'a';
	}
}
