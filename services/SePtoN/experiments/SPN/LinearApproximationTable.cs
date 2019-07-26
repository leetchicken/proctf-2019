using System;
using System.Collections.Generic;
using System.Text;

namespace SPN
{
	class LinearApproximationTable
	{
		public double[,] table;

		public LinearApproximationTable(SBox sbox)
		{
			var variantsCount = 1u << SBox.BitSize;

			table = new double[variantsCount, variantsCount];

			for(ulong X_mask = 0; X_mask < variantsCount; X_mask++)
			{
				for(ulong Y_mask = 0; Y_mask < variantsCount; Y_mask++)
				{
					int count = 0;
					for(ulong X = 0; X < variantsCount; X++)
					{
						var Y = sbox.Substitute(X);
						if(CalcLinearEquation(X_mask, Y_mask, X, Y, SBox.BitSize, SBox.BitSize))
							count++;
					}
					table[X_mask, Y_mask] = (double)count / variantsCount;
				}
			}
		}

		public static bool CalcLinearEquation(ulong X_mask, ulong Y_mask, ulong X, ulong Y, int x_mask_len, int y_mask_len)
		{
			ulong result = 0;
			for(int i = 0; i < x_mask_len; i++)
				result ^= ((X_mask >> i) & 0x1) * ((X >> i) & 0x1);

			for(int j = 0; j < y_mask_len; j++)
				result ^= ((Y_mask >> j) & 0x1) * ((Y >> j) & 0x1);

			return result == 0;
		}
	}
}
