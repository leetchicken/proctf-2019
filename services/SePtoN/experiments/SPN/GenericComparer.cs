using System;
using System.Collections.Generic;
using System.Text;

namespace SPN
{
	public class GenericComparer<T1, T2> : IEqualityComparer<T1>
	{
		public GenericComparer(Func<T1, T2> getKey) : this(getKey, EqualityComparer<T2>.Default)
		{
		}

		public GenericComparer(Func<T1, T2> getKey, IEqualityComparer<T2> comparer)
		{
			this.getKey = getKey;
			this.comparer = comparer;
		}

		public bool Equals(T1 x, T1 y)
		{
			return comparer.Equals(GetKey(x), GetKey(y));
		}

		public int GetHashCode(T1 obj)
		{
			var key = GetKey(obj);
			return (object)key == null ? 0 : comparer.GetHashCode(key);
		}

		private T2 GetKey(T1 key)
		{
			return (object)key == null ? default(T2) : getKey(key);
		}

		private readonly Func<T1, T2> getKey;
		private readonly IEqualityComparer<T2> comparer;
	}
}
