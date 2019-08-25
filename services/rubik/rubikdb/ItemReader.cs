using System;
using System.IO;

namespace rubikdb
{
	public ref struct ItemReader<T>
	{
		public ItemReader(Stream stream, Span<byte> buffer, ItemSerializer<T> serializer)
		{
			this.stream = stream;
			this.buffer = buffer;
			TotalBytesRead = 0L;
			this.serializer = serializer;
			bufferOffset = 0;
			dataLength = 0;
		}

		public bool TryReadItem(out T item)
		{
			var bytesRead = 0;

			do
			{
				dataLength += bytesRead;

				var length = serializer.TryDeserialize(buffer.Slice(bufferOffset, dataLength - bufferOffset), out item);
				if(length > 0)
				{
					bufferOffset += length;
					return true;
				}

				if(bufferOffset == 0 && dataLength == buffer.Length)
					throw new Exception("Buffer to small");

				if(bufferOffset > 0)
				{
					TotalBytesRead += bufferOffset;
					dataLength -= bufferOffset;
					if(dataLength > 0)
						buffer.Slice(bufferOffset, dataLength).CopyTo(buffer);
					bufferOffset = 0;
				}
			} while((bytesRead = stream.Read(buffer.Slice(dataLength))) > 0);

			item = default;
			return false;
		}

		public long TotalBytesRead { get; private set; }

		private readonly Stream stream;
		private readonly Span<byte> buffer;
		private readonly ItemSerializer<T> serializer;
		private int bufferOffset;
		private int dataLength;
	}
}