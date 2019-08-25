using System;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using rubikdb;

namespace rubiktest
{
	[TestClass]
	public class ItemReaderTest
	{
		[TestMethod]
		public void Test()
		{
			using var stream = new MemoryStream();
			var serializer = new UserSerializer();

			int total = 0, idx;

			Span<byte> span = stackalloc byte[32];
			for(idx = 0; idx < 10000; idx++)
			{
				var length = serializer.Serialize(new User {Login = idx.ToString(), Pass = idx.ToString(), Bio = idx.ToString(), Created = DateTime.MaxValue}, span);
				stream.Write(span.Slice(0, length));
				total += length;
			}

			var lastLength = serializer.Serialize(new User {Login = "bad", Pass = "bad", Bio = "bad", Created = DateTime.MaxValue}, span);
			stream.Write(span.Slice(lastLength - 1));

			stream.Flush();
			stream.Seek(0, SeekOrigin.Begin);

			Span<byte> buffer = stackalloc byte[32];
			var reader = new ItemReader<User>(stream, buffer, serializer);
			for(idx = 0; idx < 10000; idx++)
			{
				Assert.IsTrue(reader.TryReadItem(out var user));
				Assert.IsTrue(user != null);

				var val = idx.ToString();
				Assert.AreEqual(val, user.Login);
				Assert.AreEqual(val, user.Pass);
				Assert.AreEqual(val, user.Bio);
				Assert.AreEqual(DateTime.MaxValue, user.Created);
			}

			Assert.IsFalse(reader.TryReadItem(out _));
			Assert.AreEqual(total, reader.TotalBytesRead);
		}
	}
}
