namespace checker.rnd
{
	internal static class RndRubik
	{
		public static string RandomSolution(int minLength, int maxLength)
		{
			var length = RndUtil.GetInt(minLength, maxLength);
			var sln = new char[length];

			int chg = 0;
			var chr = Alphabet[RndUtil.ThreadStaticRnd.Next(Alphabet.Length)];
			for(int i = 0; i < length; i++)
			{
				if(i >= chg)
				{
					chg = i + 1 + RndUtil.ThreadStaticRnd.Next(4);
					chr = Alphabet[RndUtil.ThreadStaticRnd.Next(Alphabet.Length)];
				}

				sln[i] = chr;
			}

			return new string(sln);
		}

		private static readonly char[] Alphabet = "LFURBDlfurbd".ToCharArray();
	}
}
