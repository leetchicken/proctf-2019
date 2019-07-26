using System;
using System.Collections.Generic;
using System.Text;

namespace SPN
{
	static class MathUtils
	{
		public static double Bias(this double value)
		{
			return Math.Abs(0.5 - value);
		}
	}
}
