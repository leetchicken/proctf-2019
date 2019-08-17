using System;
using System.Linq;

namespace SPN
{
	public class PBox
	{
		private int[] outputBitNumbersZeroBased;
		private int[] invOutputBitNumbersZeroBased;

		//NOTE permutation bit positions in range [1..BitsCount], with 1 being the leftmost bit
		public PBox(int[] outputBitNumbers)
		{
			for(int i = 0; i < outputBitNumbers.Length; i++)
			{
				if(outputBitNumbers[i] > outputBitNumbers.Length)
					throw new ArgumentException($"outputBitNumbers[{i}] element {outputBitNumbers[i]} is out of range [1, {outputBitNumbers.Length}]");
			}

			var distinctElementsCount = outputBitNumbers.Distinct().Count();
			if(distinctElementsCount != outputBitNumbers.Length)
				throw new ArgumentException($"outputBitNumbers array is not a permutation, cause it has only {distinctElementsCount} distinct elements, not {outputBitNumbers.Length}");

			outputBitNumbersZeroBased = outputBitNumbers.Select(i => i - 1).ToArray();

			invOutputBitNumbersZeroBased = new int[outputBitNumbersZeroBased.Length];
			for(int i = 0; i < invOutputBitNumbersZeroBased.Length; i++)
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
//			if(input >> outputBitNumbersZeroBased.Length != 0)
//				throw new ArgumentException($"input {input} is out of range, must have no more than {outputBitNumbersZeroBased.Length} least significant bits set");

			ulong result = 0;
			for(int i = 0; i < bitNumbers.Length; i++)
			{
				var inputBitValue = (input >> (bitNumbers.Length - (i + 1))) & 0x1;
				var outputBitValue = inputBitValue << (bitNumbers.Length - (bitNumbers[i] + 1));
				result |= outputBitValue;
			}

			return result;
		}

	}
}