using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace SPN
{
	class SBox
	{
		public const int BitSize = 4;

		uint[] mapping = new uint[1 << BitSize];
		uint[] invmapping = new uint[1 << BitSize];

		public SBox(uint[] input, uint[] output)
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

		public uint Substitute(uint input)
		{
			if(input >= mapping.Length)
				throw new ArgumentException($"Input {input} is bigger than max allowed with BitSize {BitSize}");
			return mapping[input];
		}

		public uint InvSubstitute(uint input)
		{
			if(input >= invmapping.Length)
				throw new ArgumentException($"Input {input} is bigger than max allowed with BitSize {BitSize}");
			return invmapping[input];
		}

		public static bool IsNthBitSet(uint num, int bitNumZeroBased)
		{
			return ((num >> (BitSize - bitNumZeroBased - 1)) & 0x1) != 0;
		}

		public static uint GetUintWithNthBit(int bitNumZeroBased)
		{
			return (uint)(1 << (BitSize - bitNumZeroBased - 1));
		}
	}
}
