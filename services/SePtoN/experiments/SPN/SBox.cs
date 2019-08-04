using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SPN
{
	class SBox
	{
		public const int BitSize = 4;

		byte[] mapping = new byte[1 << BitSize];
		byte[] invmapping = new byte[1 << BitSize];

		public SBox(byte[] output) : this(Enumerable.Range(0, output.Length).Select(i => (byte)i).ToArray(), output) {}

		public SBox(byte[] input, byte[] output)
		{
			if(input.Length != output.Length)
				throw new ArgumentException($"Input and output arrays size must be equal but is not: input: ({input.Length}, output: {output.Length})");

			for(int i = 0; i < input.Length; i++)
			{
				if(input[i] >> BitSize != 0)
					throw new ArgumentException($"Invalid input[{i}] element {input[i]}. Must have no more than {BitSize} least significant bits set");
				if(output[i] >> BitSize != 0)
					throw new ArgumentException($"Invalid output[{i}] element {output[i]}. Must have no more than {BitSize} least significant bits set");

				mapping[input[i]] = output[i];
				invmapping[output[i]] = input[i];
			}
		}

		public ulong Substitute(ulong input)
		{
			if(input >= (ulong)mapping.Length)
				throw new ArgumentException($"Input {input} is bigger than max allowed with BitSize {BitSize}");
			return mapping[input];
		}

		public ulong InvSubstitute(ulong input)
		{
			if(input >= (ulong)invmapping.Length)
				throw new ArgumentException($"Input {input} is bigger than max allowed with BitSize {BitSize}");
			return invmapping[input];
		}

		public static bool IsNthBitSet(ulong num, int bitNumZeroBased)
		{
			return ((num >> (BitSize - bitNumZeroBased - 1)) & 0x1) != 0;
		}

		public static ulong GetUlongWithNthBit(int bitNumZeroBased)
		{
			return (ulong)(1 << (BitSize - bitNumZeroBased - 1));
		}
	}
}
