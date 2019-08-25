using System;
using System.Collections.Generic;
using System.Text;

namespace SPN
{
	public static class ConvertUtils
	{
		public static string ToBase64String(this string str)
		{
			return Convert.ToBase64String(Encoding.UTF8.GetBytes(str));
		}

		public static string ToStringFromBase64(this string str)
		{
			return Encoding.UTF8.GetString(Convert.FromBase64String(str));
		}
	}
}
