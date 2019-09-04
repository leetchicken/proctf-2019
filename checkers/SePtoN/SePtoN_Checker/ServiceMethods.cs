using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Numerics;
using System.Security.Cryptography;
using System.Text;
using System.Xml.Serialization;
using SPN;

namespace SePtoN_Checker
{
	class ServiceMethods
	{
		public static int ProcessCheck(string host)
		{
			var sw = Stopwatch.StartNew();
			Console.WriteLine("Processing Check");

			Console.WriteLine("Nothing to do");

			Console.WriteLine($"Check done in {sw.Elapsed}");
			return (int)ExitCode.OK;
		}


		public static int ProcessPut(string host, string id, string flag)
		{
			try
			{
				var tcpClient = ConnectToService(host, PUT_PORT);

				using(var stream = tcpClient.GetStream())
				{
					var xA = DH.GenerateRandomXA(DH_x_bitsCount);
					var yA = DH.CalculateYA(xA);
					stream.WriteBigEndian(yA);

					var yB = stream.ReadBigIntBigEndian(NetworkStreamExtensions.DH_y_bytesCount);
					var keyMaterial = DH.DeriveKey(yB, xA).ToByteArray(isBigEndian:true, isUnsigned:true);
					var masterKey = SubstitutionPermutationNetwork.CalcMasterKey(keyMaterial);

					var flagPicture = FlagsPainter.DrawFlag(flag);

					var spn = new SubstitutionPermutationNetwork(masterKey);
					var encryptedData = spn.EncryptWithPadding(flagPicture, SubstitutionPermutationNetwork.GenerateRandomIV());

					stream.WriteLengthFieldAware(encryptedData);

					var imageId = stream.ReadIntBigEndian();

					Console.Error.WriteLine($"Got image id {imageId}");

					string imageHash;
					try
					{
						imageHash = CalcImageBytesHash(flagPicture);
					}
					catch(Exception e)
					{
						throw new ServiceException(ExitCode.CHECKER_ERROR, "Failed to calc hash of generated image", innerException: e);
					}

					var state = new State
					{
						FileId = imageId,
						MasterKeyHex = Convert.ToBase64String(masterKey),
						SourceImageHash = imageHash
					};

					Console.WriteLine(state.ToJsonString().ToBase64String());
					return (int)ExitCode.OK;
				}
			}
			catch(ServiceException)
			{
				throw;
			}
			catch(Exception e)
			{
				throw new ServiceException(ExitCode.DOWN, string.Format("General failure"), innerException: e);
			}
		}

		private static string CalcImageBytesHash(byte[] flagPicture)
		{
			var ms = new MemoryStream();
			using(var bmp = new Bitmap(Image.FromStream(new MemoryStream(flagPicture))))
			{
				for(int x = 0; x < bmp.Width; x++)
				{
					for(int y = 0; y < bmp.Height; y++)
					{
						ms.WriteByte((byte)(bmp.GetPixel(x, y).R & 0xFE));
					}
				}
				return MD5.Create().ComputeHash(ms.ToArray()).ToHex();
			}
		}

		public static int ProcessGet(string host, string id, string flag)
		{
			var state = JsonHelper.ParseJson<State>(id.ToStringFromBase64());
			var imageId = state.FileId;

			var spn = new SubstitutionPermutationNetwork(Convert.FromBase64String(state.MasterKeyHex));

			try
			{
				var tcpClient = ConnectToService(host, GET_PORT);
				using(var stream = tcpClient.GetStream())
				{
					stream.WriteBigEndian(imageId);

					byte[] encryptedData = null;
					byte[] imageData;
					try
					{
						encryptedData = stream.ReadLengthFieldAware();
						imageData = spn.DecryptWithPadding(encryptedData);
					}
					catch(Exception e)
					{
						throw new ServiceException(ExitCode.CORRUPT, $"Can't receive or decrypt image of '{encryptedData?.Length}' bytes", innerException: e);
					}

					string receivedImageMd5Hex;
					try
					{
						receivedImageMd5Hex = CalcImageBytesHash(imageData);
					}
					catch(Exception e)
					{
						throw new ServiceException(ExitCode.MUMBLE, "Failed to parse image from service response", innerException: e);
					}

					if(receivedImageMd5Hex != state.SourceImageHash)
						throw new ServiceException(ExitCode.CORRUPT, $"Source image md5 {state.SourceImageHash} != received {receivedImageMd5Hex}");

					return (int)ExitCode.OK;
				}
			}
			catch(ServiceException)
			{
				throw;
			}
			catch(Exception e)
			{
				throw new ServiceException(ExitCode.DOWN, string.Format("General failure"), innerException: e);
			}
		}

		private static TcpClient ConnectToService(string host, int port)
		{
			var tcpClient = new TcpClient();
			tcpClient.ReceiveTimeout = tcpClient.SendTimeout = NetworkOperationTimeout;

			var connectedSuccessfully = tcpClient.ConnectAsync(IPAddress.Parse(host), port).Wait(ConnectTimeout);
			if(!connectedSuccessfully)
				throw new ServiceException(ExitCode.DOWN, $"Failed to connect to service in timeout {ConnectTimeout}");

			return tcpClient;
		}


		const int DH_x_bitsCount = 20 * 8;

		private const int NetworkOperationTimeout = 3000;
		private const int ConnectTimeout = 1000;

		public const int PUT_PORT = 31337;
		public const int GET_PORT = 31338;
	}

	static class NetworkStreamExtensions
	{
		public const int DH_y_bytesCount = 128 * 1 + 1;

		public static int ReadIntBigEndian(this NetworkStream stream)
		{
			var data = stream.ReadLengthFieldAware();
			if(data.Length == 0 || data.Length != sizeof(int))
				throw new ServiceException(ExitCode.MUMBLE, $"Expected {sizeof(int)} bytes of int, but got {data.Length}");
			return BitConverter.ToInt32(data.Reverse().ToArray());
		}

		public static void WriteBigEndian(this NetworkStream stream, int integer)
		{
			var buffer = BitConverter.GetBytes(integer).Reverse().ToArray();
			stream.WriteLengthFieldAware(buffer);
		}

		public static void WriteBigEndian(this NetworkStream stream, BigInteger bigInteger)
		{
			var buffer = bigInteger.ToByteArray(isBigEndian: true);
			stream.WriteLengthFieldAware(buffer);
		}

		public static BigInteger ReadBigIntBigEndian(this NetworkStream stream, int maxBytesCount)
		{
			var data = stream.ReadLengthFieldAware();
			if(data.Length == 0 || data.Length > maxBytesCount)
				throw new ServiceException(ExitCode.MUMBLE, $"Expected non-negative amount of no more than {maxBytesCount} bytes, but got {data.Length}");

			return new BigInteger(data, isUnsigned:true, isBigEndian: true);
		}

		public static void WriteLengthFieldAware(this NetworkStream stream, byte[] data)
		{
			if(data.Length == 0 || data.Length > 1 << (sizeof(ushort) * 8))
				throw new Exception($"Unable to write length-prepended array of size {data.Length}");

			ushort length = (ushort)data.Length;
			stream.Write(BitConverter.GetBytes(length).Reverse().ToArray()); //NOTE making it BigEndian
			stream.Write(data);
			stream.Flush();
		}

		public static byte[] ReadLengthFieldAware(this NetworkStream stream)
		{
			byte[] lengthBuffer = new byte[sizeof(ushort)];

			var b0 = stream.ReadByte();
			if(b0 == -1)
				throw new ServiceException(ExitCode.MUMBLE, $"Can't read message length's first byte");

			var b1 = stream.ReadByte();
			if(b1 == -1)
				throw new ServiceException(ExitCode.MUMBLE, $"Can't read message length's second byte");

			lengthBuffer[0] = (byte)b0;
			lengthBuffer[1] = (byte)b1;
			var length = BitConverter.ToUInt16(lengthBuffer.Reverse().ToArray()); //NOTE making it BigEndian

			var result = new byte[length];
			if(length == 0)
				return result;

			var leftBytes = length;

			int readBytes;
			do 
			{
				readBytes = stream.Read(new Span<byte>(result, length - leftBytes, leftBytes));
				leftBytes = (ushort)(leftBytes - readBytes);
			} while(leftBytes > 0 && readBytes > 0);

			if(leftBytes > 0)
				throw new ServiceException(ExitCode.MUMBLE, $"Expected {leftBytes} bytes but not received all of them. Have {leftBytes} left");

			return result;
		}
	}
}
