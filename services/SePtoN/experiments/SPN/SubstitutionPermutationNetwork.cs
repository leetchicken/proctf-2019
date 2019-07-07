using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace SPN
{
	class SubstitutionPermutationNetwork
	{
		public const int RoundsCount = 5; //NOTE can't be less than 2
		public const int RoundSBoxesCount = BlockSizeBytes * 8 /SBox.BitSize;
		public SBox[][] sboxes;

//		private static uint[] SBoxInput = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };
//		private static uint[] SBoxOutput = { 0xE, 0x4, 0x1, 0x2, 0xF, 0xB, 0x8, 0xD, 0x3, 0x6, 0xC, 0xA, 0x5, 0x9, 0x0, 0x7 };
//		public const int KeySizeBytes = 2;
//		private const int BlockSizeBytes = 2;
//		private static int[] PBoxOutput = { 1, 12, 7, 15, 16, 11, 9, 13, 10, 14, 8, 2, 3, 4, 5, 6 };

//		private static uint[] SBoxInput = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };
//		private static uint[] SBoxOutput = { 0xE, 0x4, 0x1, 0x2, 0xF, 0xB, 0x8, 0xD, 0x3, 0x6, 0xC, 0xA, 0x5, 0x9, 0x0, 0x7 };
//		public const int KeySizeBytes = 4;
//		private const int BlockSizeBytes = 4;
//		private static int[] PBoxOutput = { 21, 17, 19, 16, 30, 3, 4, 6, 8, 13, 29, 11, 1, 5, 7, 27, 22, 28, 20, 12, 18, 10, 14, 23, 24, 2, 15, 26, 25, 31, 32, 9 };

		private static uint[] SBox1 = {0x4, 0xA, 0x9, 0x2, 0xD, 0x8, 0x0, 0xE, 0x6, 0xB, 0x1, 0xC, 0x7, 0xF, 0x5, 0x3};
		private static uint[] SBox2 = {0xE, 0xB, 0x4, 0xC, 0x6, 0xD, 0xF, 0xA, 0x2, 0x3, 0x8, 0x1, 0x0, 0x7, 0x5, 0x9};
		private static uint[] SBox3 = {0x5, 0x8, 0x1, 0xD, 0xA, 0x3, 0x4, 0x2, 0xE, 0xF, 0xC, 0x7, 0x6, 0x0, 0x9, 0xB};
		private static uint[] SBox4 = {0x7, 0xD, 0xA, 0x1, 0x0, 0x8, 0x9, 0xF, 0xE, 0x4, 0x6, 0xC, 0xB, 0x2, 0x5, 0x3};
		private static uint[] SBox5 = {0x6, 0xC, 0x7, 0x1, 0x5, 0xF, 0xD, 0x8, 0x4, 0xA, 0x9, 0xE, 0x0, 0x3, 0xB, 0x2};
		private static uint[] SBox6 = {0x4, 0xB, 0xA, 0x0, 0x7, 0x2, 0x1, 0xD, 0x3, 0x6, 0x8, 0x5, 0x9, 0xC, 0xF, 0xE};
		private static uint[] SBox7 = {0xD, 0xB, 0x4, 0x1, 0x3, 0xF, 0x5, 0x9, 0x0, 0xA, 0xE, 0x7, 0x6, 0x8, 0x2, 0xC};
		private static uint[] SBox8 = {0x1, 0xF, 0xD, 0x0, 0x5, 0x7, 0xA, 0x4, 0x9, 0x2, 0x3, 0xE, 0x6, 0xB, 0x8, 0xC};
		private static uint[][] SBoxes = { SBox1, SBox2, SBox3, SBox4, SBox5, SBox6, SBox7, SBox8 };

		public const int KeySizeBytes = 4;
		private const int BlockSizeBytes = 4;
		private static int[] PBoxOutput = { 21, 17, 19, 16, 30, 3, 4, 6, 8, 13, 29, 11, 1, 5, 7, 27, 22, 28, 20, 12, 18, 10, 14, 23, 24, 2, 15, 26, 25, 31, 32, 9 };

		public static byte[] GenerateRandomKey()
		{
			var key = new byte[KeySizeBytes];
			new RNGCryptoServiceProvider().GetBytes(key);
			return key;
		}

		public SubstitutionPermutationNetwork(byte[] masterKey)
		{
			if(masterKey.Length != KeySizeBytes)
				throw new Exception($"Key length {masterKey.Length} is not equal to expected {KeySizeBytes} bytes");

			this.masterKey = masterKey;
			subkeys = KeyShedule(masterKey, RoundsCount).ToArray();
			sboxes = GenerateSBoxes();
			pbox = new PBox(PBoxOutput);
		}

		private SBox[][] GenerateSBoxes()
		{
			var sBoxInput = Enumerable.Range(0, 0x10).Select(i => (uint)i).ToArray();

			var result = new SBox[RoundsCount][];
			for(int i = 0; i < RoundsCount; i++)
			{
				result[i] = new SBox[RoundSBoxesCount];
				for(int j = 0; j < RoundSBoxesCount; j++)
				{
					result[i][j] = new SBox(sBoxInput, SBoxes[j]);
				}
			}
			return result;
		}

		public IEnumerable<byte[]> KeyShedule(byte[] masterKey, int roundsCount)
		{
			for(int i = 0; i < roundsCount + 1; i++)
				yield return masterKey.ToArray();
		}

		public byte[] Encrypt(byte[] block)
		{
			if(block.Length != BlockSizeBytes)
				throw new Exception($"Input block length {block.Length} is not equal to expected {BlockSizeBytes} bytes");

			var result = block.ToArray();
			for(int i = 0; i < RoundsCount; i++)
				result = EncryptRound(result, subkeys[i], sboxes[i], i == RoundsCount - 1);

			XorWithLastSubKey(result);
			return result;
		}

		public byte[] EncryptRound(byte[] input, byte[] subKey, SBox[] sboxes, bool skipPermutation = false)
		{
			var result = input.ToArray();

			for(int i = 0; i < result.Length; i++)
				result[i] = (byte)(result[i] ^ subKey[i]);

			for(int i = 0; i < result.Length; i++)
			{
				var leftPart = (byte)(result[i] >> 4);
				var rightPart = (byte)(result[i] & 0x0F);

				leftPart = (byte)sboxes[i * 2].Substitute(leftPart);
				rightPart =  (byte)sboxes[i * 2 + 1].Substitute(rightPart);

				result[i] = (byte)((leftPart << 4) | rightPart);
			}

			if(!skipPermutation)
			{
				uint pboxInput = 0;
				for(int i = 0; i < result.Length; i++)
				{
					pboxInput <<= 8;
					pboxInput |= result[i];
				}
				var pboxOutput = pbox.Permute(pboxInput);

				for(int i = 0; i < result.Length; i++)
				{
					result[result.Length - i - 1] = (byte)(pboxOutput & 0xFF);
					pboxOutput >>= 8;
				}
			}

			return result;
		}

		public byte[] Decrypt(byte[] block)
		{
			if(block.Length != BlockSizeBytes)
				throw new Exception($"Input block size {block.Length} is not equal to expected {BlockSizeBytes} bytes");

			var result = block.ToArray();

			XorWithLastSubKey(result);

			for(int i = RoundsCount - 1; i >= 0; i--)
				result = DecryptRound(result, subkeys[i], sboxes[i], i == RoundsCount - 1);


			return result;
		}

		public void XorWithLastSubKey(byte[] result)
		{
			for(int i = 0; i < result.Length; i++)
				result[i] = (byte)(result[i] ^ subkeys[RoundsCount][i]);
		}

		public byte[] DecryptRound(byte[] input, byte[] subKey, SBox[] sboxes, bool skipPermutation = false, bool skipKeyXoring = false)
		{
			var result = input.ToArray();

			if(!skipPermutation)
			{
				uint pBoxInput = 0;
				for(int i = 0; i < result.Length; i++)
				{
					pBoxInput <<= 8;
					pBoxInput |= result[i];
				}
				var pboxOutput = pbox.InvPermute(pBoxInput);

				for(int i = 0; i < result.Length; i++)
				{
					result[result.Length - i - 1] = (byte)(pboxOutput & 0xFF);
					pboxOutput >>= 8;
				}
			}
			

			for(int i = 0; i < result.Length; i++)
			{
				var leftPart = (byte)(result[i] >> 4);
				var rightPart = (byte)(result[i] & 0x0F);

				leftPart = (byte)sboxes[i * 2].InvSubstitute(leftPart);
				rightPart = (byte)sboxes[i * 2 + 1].InvSubstitute(rightPart);

				result[i] = (byte)((leftPart << 4) | rightPart);
			}

			if(!skipKeyXoring)
				for(int i = 0; i < result.Length; i++)
					result[i] = (byte)(result[i] ^ subKey[i]);

			return result;
		}

		public static uint GetBitMask(int sboxNumZeroBased, uint value)
		{
			return value << (RoundSBoxesCount - sboxNumZeroBased - 1) * SBox.BitSize;
		}

		public static uint GetSBoxMaskWithNthBit(int sboxNumZeroBased)
		{
			return (uint)(1 << (RoundSBoxesCount - sboxNumZeroBased - 1));
		}

		public static uint GetBitMaskWithNthSboxAndMthBit(int sboxNumZeroBased, int bitNumZeroBased)
		{
			return (uint)(1 << (SBox.BitSize * (RoundSBoxesCount - sboxNumZeroBased - 1) + (SBox.BitSize - bitNumZeroBased - 1)));
		}

		public static int CountBits(uint bitmask)
		{
			int result = 0;
			for(int i = 0; i < RoundSBoxesCount * SBox.BitSize; i++)
			{
				if((bitmask & 0x1) != 0)
					result++;
				bitmask >>= 1;
			}
			return result;
		}

		public static int CountBitsInSboxesMask(uint bitmask)
		{
			int result = 0;
			for(int i = 0; i < RoundSBoxesCount; i++)
			{
				if((bitmask & 0x1) != 0)
					result++;
				bitmask >>= 1;
			}
			return result;
		}

		public static List<int> GetSboxesNumsFromMask(uint bitmask)
		{
			var result = new List<int>();
			for(int i = 0; i < RoundSBoxesCount; i++)
			{
				if((bitmask & (0x1 << (RoundSBoxesCount - i - 1))) != 0)
					result.Add(i);
			}
			return result;
		}

		public static byte[] GetBytesBigEndian(uint num)
		{
			var result = new byte[BlockSizeBytes];
			for(int i = result.Length - 1; i >= 0; i--)
			{
				result[i] = (byte)(num & 0xFF);
				num >>= 8;
			}
			return result;
		}

		public static uint GetUintBigEndian(byte[] bytes)
		{
			uint result = 0;
			foreach(var b in bytes)
			{
				result <<= 8;
				result |= b;
			}
			return result;
		}

		public static string GetSboxesMaskBitString(uint bitmask)
		{
			var sb = new StringBuilder();
			for(int i = 0; i < RoundSBoxesCount; i++)
			{
				if((bitmask & (0x1 << (RoundSBoxesCount - i - 1))) != 0)
					sb.Append("1");
				else
					sb.Append("0");
			}
			return sb.ToString();
		}

		public PBox pbox;

		public readonly byte[][] subkeys;

		private readonly byte[] masterKey;
	}

}
