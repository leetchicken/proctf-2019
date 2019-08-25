using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Threading;

namespace rubikdb
{
	class RubikStore<T>
	{
		public RubikStore(string filename, ItemSerializer<T> serializer, Action<T> add)
		{
			this.serializer = serializer;
			var filepath = Path.Combine(Directory.GetCurrentDirectory(), filename);
			Directory.CreateDirectory(Path.GetDirectoryName(filepath));
			stream = new BufferedStream(new FileStream(filepath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.Read, BufferSize, FileOptions.SequentialScan), BufferSize);
			TryLoad(add);
			new Thread(FlushLoop) {IsBackground = true}.Start();
		}

		[MethodImpl(MethodImplOptions.NoOptimization)]
		public void Init() {}

		public void Write(T item)
		{
			Span<byte> span = stackalloc byte[MaxItemSize];
			var length = serializer.Serialize(item, span);
			lock(stream)
				stream.Write(span.Slice(0, length));
			Interlocked.Increment(ref written);
		}

		public void Flush()
		{
			lock(stream)
				stream.Flush();
		}

		private void TryLoad(Action<T> load)
		{
			Console.Error.WriteLine($"Loading {typeof(T).Name}s db...");
			var sw = Stopwatch.StartNew();
			Span<byte> buffer = new byte[BufferSize];
			var reader = new ItemReader<T>(stream, buffer, serializer);
			while(reader.TryReadItem(out var item))
				load(item);
			stream.SetLength(reader.TotalBytesRead);
			Console.Error.WriteLine($"{typeof(T).Name}s db loaded in {0}", sw.Elapsed);
		}

		private void FlushLoop()
		{
			while(true)
			{
				Thread.Sleep(FlushPeriod);
				if(Interlocked.Exchange(ref written, 0) == 0)
					continue;
				Flush();
			}
		}

		private const int MaxItemSize = 512;
		private const int FlushPeriod = 3000;
		private const int BufferSize = 128 * 1024;
		private readonly ItemSerializer<T> serializer;
		private readonly Stream stream;
		private int written;
	}
}
