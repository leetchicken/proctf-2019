using System;

namespace rubikdb
{
	public abstract class ItemSerializer<T>
	{
		public int Serialize(T item, Span<byte> span)
		{
			var bytes = SerializeItem(item, span.Slice(sizeof(ushort)));
			BitConverter.TryWriteBytes(span, (ushort)bytes);
			return bytes + sizeof(ushort);
		}

		public int TryDeserialize(Span<byte> span, out T item)
		{
			item = default;
			if(span.Length < sizeof(ushort))
				return 0;
			var itemLength = BitConverter.ToUInt16(span);
			var length = itemLength + sizeof(ushort);
			if(span.Length < length)
				return 0;
			if(itemLength > 0)
				item = DeserializeItem(span.Slice(sizeof(ushort), itemLength));
			return length;
		}

		protected abstract int SerializeItem(T item, Span<byte> span);
		protected abstract T DeserializeItem(Span<byte> span);
	}

	public class UserSerializer : ItemSerializer<User>
	{
		protected override int SerializeItem(User user, Span<byte> span)
			=> new BinPack(span).Write(user.Login).Write(user.Pass).Write(user.Bio).Write(user.Created).AsSpan().Length;

		protected override User DeserializeItem(Span<byte> span)
		{
			var unpack = new BinUnpack(span);
			return new User
			{
				Login = unpack.ReadString(),
				Pass = unpack.ReadString(),
				Bio = unpack.ReadString(),
				Created = unpack.ReadDateTime()
			};
		}
	}

	public class SolutionSerializer : ItemSerializer<Solution>
	{
		protected override int SerializeItem(Solution sln, Span<byte> span)
			=> new BinPack(span).Write(sln.Id).Write(sln.Login).Write(sln.MovesCount).Write(sln.Time).Write(sln.Created).AsSpan().Length;

		protected override Solution DeserializeItem(Span<byte> span)
		{
			var unpack = new BinUnpack(span);
			return new Solution
			{
				Id = unpack.ReadGuid(),
				Login = unpack.ReadString(),
				MovesCount = unpack.ReadInt32(),
				Time = unpack.ReadUInt64(),
				Created = unpack.ReadDateTime()
			};
		}
	}
}