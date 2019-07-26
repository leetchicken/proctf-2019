using System;
using System.Collections.Generic;
using System.Dynamic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace SPN
{
	class SubstitutionPermutationNetwork
	{
		public static int RoundSBoxesCount => SBoxes.Length;
		public static int BlockSizeBytes => (SBoxes.Length * SBox.BitSize) / 8;

		public const int RoundsCount = 5; //NOTE can't be less than 2

//		private static uint[] SBoxDES = { 0xE, 0x4, 0xD, 0x1, 0x2, 0xF, 0xB, 0x8, 0x3, 0xA, 0x6, 0xC, 0x5, 0x9, 0x0, 0x7 };
//		private static uint[][] SBoxes = { SBoxDES, SBoxDES, SBoxDES, SBoxDES };

//		private static uint[] SBoxLeftBit = { 0xE, 0x3, 0xC, 0xF, 0x0, 0xD, 0x1, 0x2, 0x7, 0x4, 0x6, 0x5, 0x9, 0xA, 0xB, 0x8 };
//		private static uint[][] SBoxes = { SBoxLeftBit, SBoxLeftBit, SBoxLeftBit, SBoxLeftBit };

//		private static uint[] SBoxAlmostASCII = { 0x7, 0x3, 0xC, 0xF, 0x0, 0xD, 0x1, 0x2, 0xE, 0x4, 0x6, 0x5, 0x9, 0xA, 0xB, 0x8 };
//		private static uint[][] SBoxes = { SBoxAlmostASCII, SBoxAlmostASCII, SBoxAlmostASCII, SBoxAlmostASCII };
		
		
//		private static int[] PBoxOutput = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
//		private static int[] PBoxOutput = { 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16 };



		//NOTE SBoxes from GOST 28147-89
//		private static uint[] SBox1 = {0x4, 0xA, 0x9, 0x2, 0xD, 0x8, 0x0, 0xE, 0x6, 0xB, 0x1, 0xC, 0x7, 0xF, 0x5, 0x3};
//		private static uint[] SBox2 = {0xE, 0xB, 0x4, 0xC, 0x6, 0xD, 0xF, 0xA, 0x2, 0x3, 0x8, 0x1, 0x0, 0x7, 0x5, 0x9};
//		private static uint[] SBox3 = {0x5, 0x8, 0x1, 0xD, 0xA, 0x3, 0x4, 0x2, 0xE, 0xF, 0xC, 0x7, 0x6, 0x0, 0x9, 0xB};
//		private static uint[] SBox4 = {0x7, 0xD, 0xA, 0x1, 0x0, 0x8, 0x9, 0xF, 0xE, 0x4, 0x6, 0xC, 0xB, 0x2, 0x5, 0x3};
//		private static uint[] SBox5 = {0x6, 0xC, 0x7, 0x1, 0x5, 0xF, 0xD, 0x8, 0x4, 0xA, 0x9, 0xE, 0x0, 0x3, 0xB, 0x2};
//		private static uint[] SBox6 = {0x4, 0xB, 0xA, 0x0, 0x7, 0x2, 0x1, 0xD, 0x3, 0x6, 0x8, 0x5, 0x9, 0xC, 0xF, 0xE};
//		private static uint[] SBox7 = {0xD, 0xB, 0x4, 0x1, 0x3, 0xF, 0x5, 0x9, 0x0, 0xA, 0xE, 0x7, 0x6, 0x8, 0x2, 0xC};
//		private static uint[] SBox8 = {0x1, 0xF, 0xD, 0x0, 0x5, 0x7, 0xA, 0x4, 0x9, 0x2, 0x3, 0xE, 0x6, 0xB, 0x8, 0xC};
//		private static uint[][] SBoxes = { SBox1, SBox2, SBox3, SBox4, SBox5, SBox6, SBox7, SBox8 };
//		private static int[] PBoxOutput = { 21, 17, 19, 16, 30, 3, 4, 6, 8, 13, 29, 11, 1, 5, 7, 27, 22, 28, 20, 12, 18, 10, 14, 23, 24, 2, 15, 26, 25, 31, 32, 9 };



		private static ulong[] SBox1 = {0x4, 0xA, 0x9, 0x2, 0xD, 0x8, 0x0, 0xE, 0x6, 0xB, 0x1, 0xC, 0x7, 0xF, 0x5, 0x3};
		private static ulong[] SBox2 = {0xE, 0xB, 0x4, 0xC, 0x6, 0xD, 0xF, 0xA, 0x2, 0x3, 0x8, 0x1, 0x0, 0x7, 0x5, 0x9};
		private static ulong[] SBox3 = {0x5, 0x8, 0x1, 0xD, 0xA, 0x3, 0x4, 0x2, 0xE, 0xF, 0xC, 0x7, 0x6, 0x0, 0x9, 0xB};
		private static ulong[] SBox4 = {0x7, 0xD, 0xA, 0x1, 0x0, 0x8, 0x9, 0xF, 0xE, 0x4, 0x6, 0xC, 0xB, 0x2, 0x5, 0x3};
		private static ulong[] SBox5 = {0x6, 0xC, 0x7, 0x1, 0x5, 0xF, 0xD, 0x8, 0x4, 0xA, 0x9, 0xE, 0x0, 0x3, 0xB, 0x2};
		private static ulong[] SBox6 = {0x4, 0xB, 0xA, 0x0, 0x7, 0x2, 0x1, 0xD, 0x3, 0x6, 0x8, 0x5, 0x9, 0xC, 0xF, 0xE};
		private static ulong[] SBox7 = {0xD, 0xB, 0x4, 0x1, 0x3, 0xF, 0x5, 0x9, 0x0, 0xA, 0xE, 0x7, 0x6, 0x8, 0x2, 0xC};
		private static ulong[] SBox8 = {0x1, 0xF, 0xD, 0x0, 0x5, 0x7, 0xA, 0x4, 0x9, 0x2, 0x3, 0xE, 0x6, 0xB, 0x8, 0xC};
		private static ulong[] SBox9 = { 0xC, 0x4, 0x6, 0x2, 0xA, 0x5, 0xB, 0x9, 0xE, 0x8, 0xD, 0x7, 0x0, 0x3, 0xF, 0x1 };
		private static ulong[] SBox10 = { 0x6, 0x8, 0x2, 0x3, 0x9, 0xA, 0x5, 0xC, 0x1, 0xE, 0x4, 0x7, 0xB, 0xD, 0x0, 0xF };
		private static ulong[] SBox11 = { 0xB, 0x3, 0x5, 0x8, 0x2, 0xF, 0xA, 0xD, 0xE, 0x1, 0x7, 0x4, 0xC, 0x9, 0x6, 0x0 };
		private static ulong[] SBox12 = { 0xC, 0x8, 0x2, 0x1, 0xD, 0x4, 0xF, 0x6, 0x7, 0x0, 0xA, 0x5, 0x3, 0xE, 0x9, 0xB };
		private static ulong[] SBox13 = { 0x7, 0xF, 0x5, 0xA, 0x8, 0x1, 0x6, 0xD, 0x0, 0x9, 0x3, 0xE, 0xB, 0x4, 0x2, 0xC };
		private static ulong[] SBox14 = { 0x5, 0xD, 0xF, 0x6, 0x9, 0x2, 0xC, 0xA, 0xB, 0x7, 0x8, 0x1, 0x4, 0x3, 0xE, 0x0 };
		private static ulong[] SBox15 = { 0x8, 0xE, 0x2, 0x5, 0x6, 0x9, 0x1, 0xC, 0xF, 0x4, 0xB, 0x0, 0xD, 0xA, 0x3, 0x7 };
		private static ulong[] SBox16 = { 0x1, 0x7, 0xE, 0xD, 0x0, 0x5, 0x8, 0x3, 0x4, 0xF, 0xA, 0x6, 0x9, 0xC, 0xB, 0x2 };
		private static ulong[][] SBoxes = { SBox1, SBox2, SBox3, SBox4, SBox5, SBox6, SBox7, SBox8, SBox9, SBox10, SBox11, SBox12, SBox13, SBox14, SBox15, SBox16 };
		private static int[] PBoxOutput = { 38, 4, 15, 46, 11, 16, 33, 1, 35, 64, 51, 45, 50, 55, 27, 57, 47, 52, 43, 12, 7, 40, 42, 53, 29, 10, 56, 60, 36, 20, 58, 24, 39, 37, 26, 3, 32, 17, 22, 28, 30, 23, 63, 49, 14, 62, 19, 25, 21, 5, 9, 6, 8, 34, 18, 13, 31, 61, 44, 2, 48, 41, 54, 59 };


		public SBox[][] sboxes;
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
			var sBoxInput = Enumerable.Range(0, 0x10).Select(i => (ulong)i).ToArray();

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

		public static int KeySizeBytes => (SBoxes.Length * SBox.BitSize) / 8;
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
				ulong pboxInput = 0;
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
				DecryptRound(result, subkeys[i], sboxes[i], i == RoundsCount - 1);


			return result;
		}

		public void XorWithLastSubKey(byte[] result)
		{
			for(int i = 0; i < result.Length; i++)
				result[i] = (byte)(result[i] ^ subkeys[RoundsCount][i]);
		}

		public void DecryptRound(byte[] input, byte[] subKey, SBox[] sboxes, bool skipPermutation = false, bool skipKeyXoring = false)
		{
			if(!skipPermutation)
			{
				ulong pBoxInput = 0;
				for(int i = 0; i < input.Length; i++)
				{
					pBoxInput <<= 8;
					pBoxInput |= input[i];
				}
				var pboxOutput = pbox.InvPermute(pBoxInput);

				for(int i = 0; i < input.Length; i++)
				{
					input[input.Length - i - 1] = (byte)(pboxOutput & 0xFF);
					pboxOutput >>= 8;
				}
			}
			

			for(int i = 0; i < input.Length; i++)
			{
				var leftPart = (byte)(input[i] >> 4);
				var rightPart = (byte)(input[i] & 0x0F);

				leftPart = (byte)sboxes[i * 2].InvSubstitute(leftPart);
				rightPart = (byte)sboxes[i * 2 + 1].InvSubstitute(rightPart);

				input[i] = (byte)((leftPart << 4) | rightPart);
			}

			if(!skipKeyXoring)
				for(int i = 0; i < input.Length; i++)
					input[i] = (byte)(input[i] ^ subKey[i]);
		}

		public static ulong GetBitMask(int sboxNumZeroBased, ulong value)
		{
			return value << (RoundSBoxesCount - sboxNumZeroBased - 1) * SBox.BitSize;
		}

		public static ulong GetSBoxMaskWithNthBit(int sboxNumZeroBased)
		{
			return (ulong)(1 << (RoundSBoxesCount - sboxNumZeroBased - 1));
		}

		public static ulong GetBitMaskWithNthSboxAndMthBit(int sboxNumZeroBased, int bitNumZeroBased)
		{
			return (ulong)(1 << (SBox.BitSize * (RoundSBoxesCount - sboxNumZeroBased - 1) + (SBox.BitSize - bitNumZeroBased - 1)));
		}

		public static int CountBits(ulong num)
		{
			int result = 0;
			for(int i = 0; i < RoundSBoxesCount * SBox.BitSize; i++)
			{
				if((num & 0x1) != 0)
					result++;
				num >>= 1;
			}
			return result;
		}

		public static int CountBitsInSboxesMask(ulong bitmask)
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

		public static List<int> GetSboxesNumsFromMask(ulong bitmask)
		{
			var result = new List<int>();
			for(int i = 0; i < RoundSBoxesCount; i++)
			{
				if((bitmask & (0x1ul << (RoundSBoxesCount - i - 1))) != 0)
					result.Add(i);
			}
			return result;
		}

		public static byte[] GetBytesBigEndian(ulong num)
		{
			var result = new byte[BlockSizeBytes];
			for(int i = result.Length - 1; i >= 0; i--)
			{
				result[i] = (byte)(num & 0xFF);
				num >>= 8;
			}
			return result;
		}

		public static ulong GetUlongBigEndian(byte[] bytes)
		{
			ulong result = 0;
			foreach(var b in bytes)
			{
				result <<= 8;
				result |= b;
			}
			return result;
		}

		public static string GetSboxesMaskBitString(ulong bitmask)
		{
			var sb = new StringBuilder();
			for(int i = 0; i < RoundSBoxesCount; i++)
			{
				if((bitmask & (0x1ul << (RoundSBoxesCount - i - 1))) != 0)
					sb.Append("1");
				else
					sb.Append("0");
			}
			return sb.ToString();
		}

		public static string GetBitString(ulong num)
		{
			var sb = new StringBuilder();
			for(int i = 0; i < BlockSizeBytes * 8; i++)
			{
				if((num & (0x1ul << (BlockSizeBytes * 8 - i - 1))) != 0)
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
