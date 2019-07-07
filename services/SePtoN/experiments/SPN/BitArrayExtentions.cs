using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace SPN
{
	static class BitArrayExtentions
	{
		public static string Print(this BitArray bitArray)
		{
			StringBuilder sb = new StringBuilder();
			for(int i = 0; i < bitArray.Length; i++)
			{
				sb.Append(bitArray[i] ? "1" : "0");
			}
			return sb.ToString();
		}

		public static BitArray FromUInt(uint value)
		{
			var bools = new bool[sizeof(uint) * 8];
			for(int i = 0; i < bools.Length; i++)
			{
				bools[i] = ((value >> (bools.Length - i - 1)) & 0x1) != 0;
			}
			return new BitArray(bools);
		}
	}
}
