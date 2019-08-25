using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using rubikdb;

namespace rubiktest
{
	[TestClass]
	public class BinPackTest
	{
		[TestMethod]
		public void Test()
		{
			Span<byte> span = stackalloc byte[256];
			var pack = new BinPack(span);

			var value1 = 128;
			pack.Write(value1);

			var value2 = 0xffffffffffffffffUL;
			pack.Write(value2);

			var value3 = DateTime.MaxValue;
			pack.Write(value3);

			var value4 = new Guid("deadbeef-1234-5678-9012-abcdef012345");
			pack.Write(value4);

			var value5 = "test";
			pack.Write(value5);

			var value6 = "";
			pack.Write(value6);

			var unpack = new BinUnpack(span);
			Assert.AreEqual(value1, unpack.ReadInt32());
			Assert.AreEqual(value2, unpack.ReadUInt64());
			Assert.AreEqual(value3, unpack.ReadDateTime());
			Assert.AreEqual(value4, unpack.ReadGuid());
			Assert.AreEqual(value5, unpack.ReadString());
			Assert.AreEqual(value6, unpack.ReadString());
		}
	}
}
