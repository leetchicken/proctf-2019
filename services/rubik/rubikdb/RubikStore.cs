using System;
using System.IO;
using System.Text.Json;
using System.Threading;

namespace rubikdb
{
	class RubikStore<T>
	{
		public RubikStore(string filename, Action<T> add)
		{
			var filepath = Path.Combine(Directory.GetCurrentDirectory(), filename);
			Directory.CreateDirectory(Path.GetDirectoryName(filepath));
			stream = new BufferedStream(new FileStream(filepath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.Read, BufferSize, FileOptions.SequentialScan), BufferSize);
			TryLoad(stream, add);
			new Thread(FlushLoop) {IsBackground = true}.Start();
		}

		public void Write(T item)
		{
			lock(stream)
			{
				stream.WriteByte(0xd);
				using var writer = new Utf8JsonWriter(stream);
				JsonSerializer.Serialize(writer, item);
				stream.WriteByte(0xd);
			}
			Interlocked.Increment(ref written);
		}

		public void Flush()
		{
			lock(stream)
				stream.Flush();
		}

		private static void TryLoad(Stream stream, Action<T> load)
		{
			Span<byte> buffer = new byte[BufferSize];
			var reader = new SpanBasedStreamLineReader(stream, buffer);
			while(reader.TryReadLine(out var line))
			{
				if(line.Length == 0)
					continue;
				try
				{
					var item = JsonSerializer.Deserialize<T>(line);
					load(item);
				}
				catch { /* Broken record skipped */ }
			}
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

		private const int FlushPeriod = 3000;
		private const int BufferSize = 512 * 1024;
		private readonly Stream stream;
		private int written;
	}
}
