using System;
using System.Text;

namespace rubikdb
{
	public ref struct BinPack
	{
		public BinPack(Span<byte> span)
		{
			this.span = span;
			length = 0;
		}

		public BinPack Write(int val)
		{
			BitConverter.TryWriteBytes(span.Slice(length), val);
			length += sizeof(int);
			return this;
		}

		public BinPack Write(long val)
		{
			BitConverter.TryWriteBytes(span.Slice(length), val);
			length += sizeof(long);
			return this;
		}

		public BinPack Write(ulong val)
		{
			BitConverter.TryWriteBytes(span.Slice(length), val);
			length += sizeof(ulong);
			return this;
		}

		public BinPack Write(DateTime val) => Write(val.Ticks);

		public BinPack Write(Guid val)
		{
			val.TryWriteBytes(span.Slice(length));
			length += GuidLength;
			return this;
		}

		public BinPack Write(string value)
		{
			var bytes = Encoding.UTF8.GetBytes(value, span.Slice(length + 1, span.Length - length >= 256 ? 256 : Encoding.UTF8.GetByteCount(value)));
			span[length] = (byte)bytes;
			length += 1 + bytes;
			return this;
		}

		public Span<byte> AsSpan() => span.Slice(0, length);

		private const int GuidLength = 16;

		private readonly Span<byte> span;
		private int length;
	}

	public ref struct BinUnpack
	{
		public BinUnpack(Span<byte> span)
		{
			this.span = span;
			offset = 0;
		}

		public int ReadInt32()
		{
			var value = BitConverter.ToInt32(span.Slice(offset));
			offset += sizeof(int);
			return value;
		}

		public long ReadInt64()
		{
			var value = BitConverter.ToInt64(span.Slice(offset));
			offset += sizeof(long);
			return value;
		}

		public ulong ReadUInt64()
		{
			var value = BitConverter.ToUInt64(span.Slice(offset));
			offset += sizeof(ulong);
			return value;
		}

		public DateTime ReadDateTime() => new DateTime(ReadInt64());

		public Guid ReadGuid()
		{
			var value = new Guid(span.Slice(offset, GuidLength));
			offset += GuidLength;
			return value;
		}

		public string ReadString()
		{
			var length = span[offset];
			var value = Encoding.UTF8.GetString(span.Slice(offset + 1, length));
			offset += 1 + length;
			return value;
		}

		private const int GuidLength = 16;

		private readonly Span<byte> span;
		private int offset;
	}
}
