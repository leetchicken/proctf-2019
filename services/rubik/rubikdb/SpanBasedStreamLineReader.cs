using System;
using System.IO;

namespace rubikdb
{
	internal ref struct SpanBasedStreamLineReader
	{
		public SpanBasedStreamLineReader(Stream stream, Span<byte> buffer)
		{
			this.stream = stream;
			this.buffer = buffer;
			totalBytesRead = 0L;
			bufferOffset = 0;
			dataLength = 0;
			skipNext = false;
		}

		public bool TryReadLine(out Span<byte> line)
		{
			var bytesRead = 0;

			do
			{
				dataLength += bytesRead;
				for(var index = bufferOffset; index < dataLength; index++)
				{
					if(buffer[index] != CR && buffer[index] != LF)
						continue;

					var oldBufferOffset = bufferOffset;
					bufferOffset = index + 1;

					if(skipNext)
						skipNext = false;
					else
					{
						line = buffer.Slice(oldBufferOffset, index - oldBufferOffset);
						return true;
					}
				}

				if(bufferOffset == 0 && dataLength == buffer.Length)
				{
					Console.Error.WriteLine("Line skipped at offset {0} - too long", totalBytesRead + bufferOffset);
					bufferOffset += dataLength;
					skipNext = true;
				}

				totalBytesRead += bytesRead;

				if(bufferOffset > 0)
				{
					dataLength -= bufferOffset;
					if(dataLength > 0)
						buffer.Slice(bufferOffset, dataLength).CopyTo(buffer);
					bufferOffset = 0;
				}
			} while((bytesRead = stream.Read(buffer.Slice(dataLength))) > 0);

			line = default;
			return false;
		}

		private const byte CR = 0xd;
		private const byte LF = 0xa;
		private readonly Stream stream;
		private readonly Span<byte> buffer;
		private long totalBytesRead;
		private int bufferOffset;
		private int dataLength;
		private bool skipNext;
	}
}