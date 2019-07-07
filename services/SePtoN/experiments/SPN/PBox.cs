using System;
using System.Linq;

namespace SPN
{
	class PBox
	{
		private const int BitsCount = 32;

		private int[] outputBitNumbersZeroBased;
		private int[] invOutputBitNumbersZeroBased;

		//NOTE permutation bit positions in range [1..BitsCount], with 1 being the leftmost bit
		public PBox(int[] outputBitNumbers)
		{
			if(outputBitNumbers.Length != BitsCount)
				throw new ArgumentException($"outputBitNumbers array size ({outputBitNumbers.Length} is not equal to expected {BitsCount})");

			for(int i = 0; i < outputBitNumbers.Length; i++)
			{
				if(outputBitNumbers[i] > BitsCount)
					throw new ArgumentException($"outputBitNumbers[{i}] element {outputBitNumbers[i]} is out of range [1, {BitsCount}]");
			}

			var distinctElementsCount = outputBitNumbers.Distinct().Count();
			if(distinctElementsCount != outputBitNumbers.Length)
				throw new ArgumentException($"outputBitNumbers array is not a permutation, cause it has only {distinctElementsCount} distinct elements, not {BitsCount}");

			outputBitNumbersZeroBased = outputBitNumbers.Select(i => i - 1).ToArray();

			invOutputBitNumbersZeroBased = new int[outputBitNumbersZeroBased.Length];
			for(int i = 0; i < outputBitNumbersZeroBased.Length; i++)
				invOutputBitNumbersZeroBased[outputBitNumbersZeroBased[i]] = i;

		}

		public int PermuteBit(int bitNum)
		{
			return outputBitNumbersZeroBased[bitNum];
		}

		public int InvPermuteBit(int bitNum)
		{
			return invOutputBitNumbersZeroBased[bitNum];
		}

		public ulong Permute(ulong input)
		{
			return PermuteInternal(input, outputBitNumbersZeroBased);
		}

		public ulong InvPermute(ulong input)
		{
			return PermuteInternal(input, invOutputBitNumbersZeroBased);
		}

		private ulong PermuteInternal(ulong input, int[] bitNumbers)
		{
			if(input >> BitsCount != 0)
				throw new ArgumentException($"input {input} is out of range, must have no more than {BitsCount} least significant bits set");

			ulong result = 0;
			for(int i = 0; i < BitsCount; i++)
			{
				var inputBitValue = (input >> (BitsCount - (i + 1))) & 0x1;
				var outputBitValue = inputBitValue << (BitsCount - (bitNumbers[i] + 1));
				result |= outputBitValue;
			}

			return result;
		}

	}
}