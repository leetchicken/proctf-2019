using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SPN
{
	static class ICollectionUtils
	{
		public static IEnumerable<IEnumerable<T>> CrossJoin<T>(this ICollection<IEnumerable<T>> enumerableOfEnumerables)
		{
			return enumerableOfEnumerables.Skip(1)
				.Aggregate(enumerableOfEnumerables.First().Select(t => new List<T> { t }),
					(previous, next) => previous
						.SelectMany(p => next.Select(d => new List<T>(p) { d })));
		}

	}
}
