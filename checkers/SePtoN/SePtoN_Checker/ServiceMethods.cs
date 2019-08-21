using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Numerics;
using System.Text;
using log4net;
using SPN;

namespace SePtoN_Checker
{
	class ServiceMethods
	{
		public static int ProcessCheck(string host)
		{
			var sw = Stopwatch.StartNew();
			log.Info("Processing Check");



			log.Info($"Check done in {sw.Elapsed}");
			return (int)ExitCode.OK;
		}




		private static readonly ILog log = LogManager.GetLogger(typeof(ServiceMethods));

		public static int ProcessPut(string host, string id, string flag)
		{
			try
			{
				var tcpClient = new TcpClient();
				tcpClient.ReceiveTimeout = tcpClient.SendTimeout = NetworkOperationTimeout;

				var connectedSuccessfully = tcpClient.ConnectAsync(IPAddress.Parse(host), PORT).Wait(ConnectTimeout);
				if(!connectedSuccessfully)
					throw new ServiceException( ExitCode.DOWN, $"Failed to connect to service in timeount {ConnectTimeout}");

				using(var stream = tcpClient.GetStream())
				{
					var xA = DH.GenerateRandomXA(DH_x_bitsCount);
					var yA = DH.CalculateYA(xA);
					stream.WriteBigIntBigEndian(yA);

					var yB = stream.ReadBigIntBigEndian();
					var keyMaterial = DH.DeriveKey(yB, xA).ToByteArray(isBigEndian:true);
					var masterKey = SubstitutionPermutationNetwork.CalcMasterKey(keyMaterial);

					var flagPicture = FlagsPainter.DrawFlag(flag);
					var spn = new SubstitutionPermutationNetwork(masterKey);
					var encryptedData = spn.EncryptWithPadding(flagPicture, SubstitutionPermutationNetwork.GenerateRandomIV());

					stream.WriteLengthFieldAware(encryptedData);

					return -1;
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

		public static int ProcessGet(string host, string id, string flag)
		{
			throw new NotImplementedException();
		}

		
		const int DH_x_bitsCount = 20 * 8;

		private const int NetworkOperationTimeout = 1000;
		private const int ConnectTimeout = 1000;

		public const int PORT = 31337;
	}

	static class NetworkStreamExtensions
	{
		const int DH_y_bytesCount = 128 * 1 + 1;

		public static void WriteBigIntBigEndian(this NetworkStream stream, BigInteger bigInteger)
		{
			var buffer = bigInteger.ToByteArray(isBigEndian: true);
			stream.WriteLengthFieldAware(buffer);
		}

		public static BigInteger ReadBigIntBigEndian(this NetworkStream stream)
		{
			var data = stream.ReadLengthFieldAware();
			if(data.Length == 0 || data.Length > DH_y_bytesCount)
				throw new ServiceException(ExitCode.MUMBLE, $"Expected non-negative amount of no more than {DH_y_bytesCount} bytes of yB, but got {data.Length}");

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

			lengthBuffer[0] = (byte)stream.ReadByte();
			lengthBuffer[1] = (byte)stream.ReadByte();
			var length = BitConverter.ToUInt16(lengthBuffer.Reverse().ToArray()); //NOTE making it BigEndian

			var result = new byte[length];

			var leftBytes = length;

			while(leftBytes > 0)
			{
				var readBytes = stream.Read(new Span<byte>(result, length - leftBytes, leftBytes));
				leftBytes = (ushort)(leftBytes - readBytes);
			}

			return result;
		}
	}
}
