using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace SPN
{
	class Program
	{
		private static byte[] Key;
		static void Main(string[] args)
		{
			Console.WriteLine($"SPN: rounds {SubstitutionPermutationNetwork.RoundsCount}, blocksize {SubstitutionPermutationNetwork.BlockSizeBytes * 8 } bits");

			Key = SubstitutionPermutationNetwork.GenerateRandomKey();
			Console.WriteLine($"key: {Key.ToHexUpperCase()}");

			var spn = new SubstitutionPermutationNetwork(Key);

			var plainTextString = "HW";
			var plainText = Encoding.ASCII.GetBytes(plainTextString);
			var encryptedBytes = spn.Encrypt(plainText);
			var decryptedBytes = spn.Decrypt(encryptedBytes);
			var decryptedString = Encoding.ASCII.GetString(decryptedBytes);
			Console.WriteLine($"{plainTextString} -> {encryptedBytes.ToHexUpperCase()} -> {decryptedString}");

			HackCipher(spn);
		}

		private static void HackCipher(SubstitutionPermutationNetwork spn)
		{
			var linearCryptoanalysis = new LinearCryptoanalysis(spn);

			var thresholdBias = 0.001;
			var maxSBoxesInLastRound = 2;

			var bestApproximations = linearCryptoanalysis.ChooseBestPathsStartingFromSingleSBoxInRound0(maxSBoxesInLastRound, thresholdBias).ToList();

			foreach(var bestApproximation in bestApproximations)
			{
				HackApproximation(spn, bestApproximation);
			}
		}

		const int iterationsCount = 1_000_000;

		private static void HackApproximation(SubstitutionPermutationNetwork spn, SBoxesWithPath bestApproximation)
		{
			Console.WriteLine($"\nBEST OPTION: round0sboxNum {bestApproximation.sboxNum}\tX {bestApproximation.X}\tY {bestApproximation.Y}\tbias {Math.Abs(0.5 - bestApproximation.probability)}\tSBoxes {SubstitutionPermutationNetwork.GetSboxesMaskBitString(bestApproximation.lastRoundSBoxes)}");

			var targetPartialSubkeys = GenerateTargetPartialSubkeys(bestApproximation.lastRoundSBoxes)
										.Select(targetPartialSubkey => (targetPartialSubkey, SubstitutionPermutationNetwork.GetBytesBigEndian(targetPartialSubkey)))
										.ToList();

			var keyProbabilities = targetPartialSubkeys
									.ToDictionary(u => u.Item1, u => 0);
			var hackingSubstitutionPermutationNetwork = new SubstitutionPermutationNetwork(SubstitutionPermutationNetwork.GenerateRandomKey());

			for(int it = 0; it < iterationsCount; it++)
				HackIteration(spn, bestApproximation, targetPartialSubkeys, hackingSubstitutionPermutationNetwork, keyProbabilities);

			PrintIteration(bestApproximation, keyProbabilities);
		}

		private static void HackIteration(SubstitutionPermutationNetwork spn, SBoxesWithPath bestApproximation, List<(uint, byte[])> targetPartialSubkeys, SubstitutionPermutationNetwork hackingSubstitutionPermutationNetwork, Dictionary<uint, int> keyProbabilities)
		{
			var plainText = GenerateRandomPlainText();
			var enc = spn.Encrypt(plainText);

			var p_setBitsCount = SubstitutionPermutationNetwork.CountBits(SubstitutionPermutationNetwork.GetUintBigEndian(plainText) & (bestApproximation.X << ((SubstitutionPermutationNetwork.RoundSBoxesCount - bestApproximation.sboxNum - 1) * SBox.BitSize)));

			var encUnkeyed = new byte[enc.Length];
			foreach(var (targetPartialSubkey, subkeyBytes) in targetPartialSubkeys)
			{
				for(int i = 0; i < subkeyBytes.Length; i++)
					encUnkeyed[i] = (byte)(enc[i] ^ subkeyBytes[i]);

				hackingSubstitutionPermutationNetwork.DecryptRound(encUnkeyed, null, hackingSubstitutionPermutationNetwork.sboxes[SubstitutionPermutationNetwork.RoundsCount - 1], true, true);

				var u_setBitsCount = SubstitutionPermutationNetwork.CountBits(bestApproximation.lastRoundBits & SubstitutionPermutationNetwork.GetUintBigEndian(encUnkeyed));

				if((p_setBitsCount + u_setBitsCount) % 2 == 0)
					keyProbabilities[targetPartialSubkey]++;
			}
		}

		private static void PrintIteration(SBoxesWithPath bestApproximation, Dictionary<uint, int> keyProbabilities)
		{
			Console.WriteLine($"ITERATIONS DONE: {iterationsCount}");
			Console.ForegroundColor = ConsoleColor.Blue;
			Console.WriteLine($" {Key.ToHexUpperCase()} : REAL KEY");
			Console.ResetColor();

			int prevBias = -1;
			var prefix = "";
			var keyValuePairs = keyProbabilities.OrderByDescending(kvp => Math.Abs(iterationsCount / 2 - kvp.Value)) /*.Take(8)*/.ToList();
			foreach(var kvp in keyValuePairs)
			{
				var keyBytes = SubstitutionPermutationNetwork.GetBytesBigEndian(kvp.Key);
				var isValidKey = IsValidKey(Key, keyBytes, bestApproximation.lastRoundSBoxes);

				var bias = Math.Abs(iterationsCount / 2 - kvp.Value);

				if(bias != prevBias && prevBias != -1)
					prefix = prefix == " " ? "" : " ";

				if(isValidKey)
					Console.ForegroundColor = ConsoleColor.Green;
				Console.WriteLine($"{prefix}{keyBytes.ToHexUpperCase()} : {bias}");
				Console.ResetColor();
				prevBias = bias;
			}
		}

		private static List<uint> GenerateTargetPartialSubkeys(uint lastRoundSBoxes)
		{
			var targetPartialSubkeys = new List<uint> { 0 };

			var vulnerableLastRoundSBoxesNums = SubstitutionPermutationNetwork.GetSboxesNumsFromMask(lastRoundSBoxes);
			while(vulnerableLastRoundSBoxesNums.Count > 0)
			{
				var sBoxNum = vulnerableLastRoundSBoxesNums.Last();

				var newTargetPartialSubkeys = new List<uint>();
				foreach(var targetPartialSubkey in targetPartialSubkeys)
					for(uint v = 0; v < 1 << SBox.BitSize; v++)
						newTargetPartialSubkeys.Add(targetPartialSubkey | SubstitutionPermutationNetwork.GetBitMask(sBoxNum, v));
				targetPartialSubkeys = newTargetPartialSubkeys;

				vulnerableLastRoundSBoxesNums.RemoveAt(vulnerableLastRoundSBoxesNums.Count - 1);
			}

			return targetPartialSubkeys;
		}

		private static byte[] GenerateRandomPlainText()
		{
			var block = new byte[SubstitutionPermutationNetwork.KeySizeBytes];
			new RNGCryptoServiceProvider().GetBytes(block);
			return block;
		}

		private static bool IsValidKey(byte[] expected, byte[] got, uint lastRoundSBoxes)
		{
			var sboxesNumsFromMask = SubstitutionPermutationNetwork.GetSboxesNumsFromMask(lastRoundSBoxes);

			foreach(var sBoxNum in sboxesNumsFromMask)
			{
				var byteNum = sBoxNum / 2;
				if(((got[byteNum] ^ expected[byteNum]) & (sBoxNum % 2 == 0 ? 0xF0 : 0x0F)) != 0)
					return false;
			}
			return true;
		}
	}
}
