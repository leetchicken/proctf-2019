using System;
using System.Collections.Generic;
using System.Linq;

namespace checker.rubik
{
	internal static class SolveHelper
	{
		public static string ConvertInputCube(string puzzle)
		{
			var array = new char[54];
			for(int face = 0; face < 6; face++)
			{
				var oldIdx = face * 9;
				var newIdx = FaceIndicesTable[face] * 9;
				array[newIdx + 0] = ColorsConvertTable[puzzle[oldIdx + 0]];
				array[newIdx + 1] = ColorsConvertTable[puzzle[oldIdx + 1]];
				array[newIdx + 2] = ColorsConvertTable[puzzle[oldIdx + 2]];
				array[newIdx + 3] = ColorsConvertTable[puzzle[oldIdx + 7]];
				array[newIdx + 4] = ColorsConvertTable[puzzle[oldIdx + 8]];
				array[newIdx + 5] = ColorsConvertTable[puzzle[oldIdx + 3]];
				array[newIdx + 6] = ColorsConvertTable[puzzle[oldIdx + 6]];
				array[newIdx + 7] = ColorsConvertTable[puzzle[oldIdx + 5]];
				array[newIdx + 8] = ColorsConvertTable[puzzle[oldIdx + 4]];
			}
			return new string(array);
		}

		public static string ConvertOutputSolution(string solution)
		{
			return string.Join("", solution.Split((char[])null, StringSplitOptions.RemoveEmptyEntries).Select(part =>
			{
				if(part.Length == 1)
					return part;
				if(part[1] == '2')
					return new string(part[0], 2);
				if(part[1] == '\'')
					return char.ToLowerInvariant(part[0]).ToString();
				throw new InvalidOperationException();
			}));
		}

		private static readonly int[] FaceIndicesTable = {4, 2, 0, 1, 5, 3};
		private static readonly Dictionary<char, char> ColorsConvertTable = new Dictionary<char, char>
		{
			{'R', 'L'},
			{'G', 'F'},
			{'B', 'U'},
			{'W', 'R'},
			{'Y', 'B'},
			{'Z', 'D'}
		};
	}
}
