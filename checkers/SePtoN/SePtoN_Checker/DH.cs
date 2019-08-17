using System;
using System.Collections.Generic;
using System.Numerics;
using System.Security.Cryptography;
using System.Text;

namespace SePtoN_Checker
{
	class DH
	{
		// Params from RFC 5114
		private static BigInteger p = BigInteger.Parse("124325339146889384540494091085456630009856882741872806181731279018491820800119460022367403769795008250021191767583423221479185609066059226301250167164084041279837566626881119772675984258163062926954046545485368458404445166682380071370274810671501916789361956272226105723317679562001235501455748016154805420913");
		private static BigInteger g = BigInteger.Parse("115740200527109164239523414760926155534485715860090261532154107313946218459149402375178179458041461723723231563839316251515439564315555249353831328479173170684416728715378198172203100328308536292821245983596065287318698169565702979765910089654821728828592422299160041156491980943427556153020487552135890973413");

		public static BigInteger GenerateRandomXA(int bitWidth)
		{
			using(RNGCryptoServiceProvider rng = new RNGCryptoServiceProvider())
			{
				byte[] data = new byte[bitWidth / 8];
				rng.GetBytes(data);
				data[data.Length - 1] &= (byte)0x7F;

				return new BigInteger(data);
			}
		}

		public static BigInteger CalculateYA(BigInteger xA)
		{
			return BigInteger.ModPow(g, xA, p);
		}

		public static BigInteger DeriveKey(BigInteger yB, BigInteger xA)
		{
			return BigInteger.ModPow(yB, xA, p);
		}
	}
}
