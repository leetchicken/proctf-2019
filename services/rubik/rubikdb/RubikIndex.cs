using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;

namespace rubikdb
{
	class RubikIndex<TKey, TValue> where TValue : class
	{
		public RubikIndex(TimeSpan ttl, Func<TValue, TKey> getKey, Action<TValue> onRemove = null, IEqualityComparer<TKey> comparer = null)
		{
			this.ttl = ttl;
			this.getKey = getKey;
			this.onRemove = onRemove;
			ordered = new ReadSafeLinkedListNode<(TValue, DateTime)>.LinkedList();
			dict = new ConcurrentDictionary<TKey, TValue>(comparer ?? EqualityComparer<TKey>.Default);
			if(ttl != TimeSpan.MaxValue)
			{
				var period = TimeSpan.FromSeconds(Math.Max(1L, (long)ttl.TotalSeconds >> 3));
				timer = new Timer(RemoveExpired, null, period, period);
			}
		}

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		public TValue GetOrDefault(TKey key) => dict.TryGetValue(key, out var item) ? item : default;

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		public bool TryAdd(TValue value) => TryAdd(value, DateTime.UtcNow);

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		public bool GetOrAdd(ref TValue value) => GetOrAdd(ref value, DateTime.UtcNow);

		private bool GetOrAdd(ref TValue value, DateTime time)
		{
			var add = value;
			var key = getKey(value);
			if(!ReferenceEquals(add, value = dict.GetOrAdd(key, value)))
				return false;
			lock(ordered)
				ordered.AddFirst((value, time));
			return true;
		}

		public bool TryAdd(TValue value, DateTime time)
		{
			if(time < GetExpireTime())
				return false;
			var key = getKey(value);
			if(!dict.TryAdd(key, value))
				return false;
			lock(ordered)
				ordered.AddFirst((value, time));
			return true;
		}

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		public IEnumerable<TValue> TakeLast() => ordered.Select(dated => dated.value);

		private void RemoveExpired(object state)
		{
			var expire = GetExpireTime();
			while(ordered.Last is var last && last != null && last.Value.added < expire)
			{
				lock(ordered) ordered.RemoveLast();
				dict.TryRemove(getKey(last.Value.value), out var item);
				onRemove?.Invoke(item);
			}
		}

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		private DateTime GetExpireTime() => DateTime.UtcNow.Add(-ttl);

		private readonly ReadSafeLinkedListNode<(TValue value, DateTime added)>.LinkedList ordered;
		private readonly ConcurrentDictionary<TKey, TValue> dict;
		private readonly TimeSpan ttl;
		private readonly Func<TValue, TKey> getKey;
		private readonly Action<TValue> onRemove;
		private readonly Timer timer;
	}
}