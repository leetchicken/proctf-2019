using System;
using System.Collections.Generic;
using System.Linq;


namespace SPN
{
	static class IEnumerableExtensions
	{
		public static T MaxOrDefault<T>(this IEnumerable<T> enumerable, Func<T, double> selector)
		{
			return BestOrDefault<T>(enumerable, (arg1, arg2) => selector(arg1) > selector(arg2));
		}

		public static T BestOrDefault<T>(this IEnumerable<T> enumerable, Func<T, T, bool> firstIsBetter)
		{
			using(var enumerator = enumerable.GetEnumerator())
			{
				if(!enumerator.MoveNext())
					return default(T);
				var best = enumerator.Current;
				while(enumerator.MoveNext())
				{
					var item = enumerator.Current;
					if(firstIsBetter(item, best))
						best = item;
				}
				return best;
			}
		}

		public static IEnumerable<T> Distinct<T, TKey>(this IEnumerable<T> enumerable, Func<T, TKey> comparand)
		{
			return enumerable.Distinct(new GenericComparer<T, TKey>(comparand));
		}
	}
}
