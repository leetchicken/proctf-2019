using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace notepool
{
	public class AsyncLockPool
	{
		public AsyncLockPool(int capacity = 101) => locks = Enumerable.Range(0, capacity).Select(i => new AsyncLockSource()).ToArray();
		public AsyncLockSource GetLockObject(string key) => locks[unchecked((uint)StringComparer.InvariantCultureIgnoreCase.GetHashCode(key ?? string.Empty) % locks.Length)];
		private readonly AsyncLockSource[] locks;
	}

	public class AsyncLockSource : IDisposable
	{
		public async ValueTask<AsyncLock> AcquireAsync(CancellationToken token)
		{
			await semaphore.WaitAsync(token);
			return new AsyncLock(this);
		}

		public void Dispose() => semaphore.Dispose();
		private void Release() => semaphore.Release();

		public struct AsyncLock : IDisposable
		{
			public AsyncLock(AsyncLockSource source) => this.source = source;
			public void Dispose() => source.Release();
			private readonly AsyncLockSource source;
		}

		private readonly SemaphoreSlim semaphore = new SemaphoreSlim(1, 1);
	}
}
