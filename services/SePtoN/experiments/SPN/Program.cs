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

//			HackCipher(spn);
			HackCipher_Fixed(spn);
		}

		const double thresholdBias = 0.01;
		const int maxSBoxesInLastRound = 2;
		const int maxSBoxesInRound = 2 * maxSBoxesInLastRound;

		const int iterationsCount = 10_000;


		private static void HackCipher_Fixed(SubstitutionPermutationNetwork spn)
		{
			var linearCryptoanalysis = new LinearCryptoanalysis(spn);

			var bestLayerApproximations = linearCryptoanalysis.ChooseBestPathsStartingFromSingleSBoxInRound0_Fixed(maxSBoxesInLastRound, maxSBoxesInRound, thresholdBias).ToList();
			foreach(var bestLayerApproximation in bestLayerApproximations)
				HackApproximation_Fixed(spn, bestLayerApproximation);
		}


		private static void HackApproximation_Fixed(SubstitutionPermutationNetwork spn, Layer bestLayerApproximation)
		{
			Console.WriteLine($"\nBEST OPTION: round0sboxNum {bestLayerApproximation.round0sboxNum}\tX {bestLayerApproximation.round0x}\tY {bestLayerApproximation.round0y}\tbias {Math.Abs(0.5 - bestLayerApproximation.inputProbability)}\tSBoxes {string.Join(",", bestLayerApproximation.ActivatedSboxesNums)}\tLastRoundInputBits {SubstitutionPermutationNetwork.GetBitString(bestLayerApproximation.inputBits)}");

			var targetPartialSubkeys = GenerateTargetPartialSubkeys(bestLayerApproximation.ActivatedSboxesNums)
				.Select(targetPartialSubkey => (targetPartialSubkey, SubstitutionPermutationNetwork.GetBytesBigEndian(targetPartialSubkey)))
				.ToList();

			var keyProbabilities = targetPartialSubkeys
				.ToDictionary(u => u.Item1, u => 0);
			var hackingSubstitutionPermutationNetwork = new SubstitutionPermutationNetwork(SubstitutionPermutationNetwork.GenerateRandomKey());

			for(int it = 0; it < iterationsCount; it++)
			{
				if(it > 0 && it % (iterationsCount / 4) == 0)
				{
					Console.WriteLine($" done {it} iterations of {iterationsCount}");
				}
				HackIteration(spn, bestLayerApproximation.round0sboxNum, bestLayerApproximation.round0x, bestLayerApproximation.inputBits, targetPartialSubkeys, hackingSubstitutionPermutationNetwork, keyProbabilities);
			}

			PrintApproximation_Fixed(bestLayerApproximation, keyProbabilities);
		}

		private static void HackCipher(SubstitutionPermutationNetwork spn)
		{
			var linearCryptoanalysis = new LinearCryptoanalysis(spn);
			var bestApproximations = linearCryptoanalysis.ChooseBestPathsStartingFromSingleSBoxInRound0(maxSBoxesInLastRound, thresholdBias).ToList();
			var bestApproximationsForASCII = bestApproximations
				//												.Where(sBoxesWithPath => sBoxesWithPath.sboxNum % 2 == 0 && sBoxesWithPath.X == 8).ToList()
				;

			foreach(var bestApproximation in bestApproximationsForASCII)
				HackApproximation(spn, bestApproximation);
		}

		private static void HackApproximation(SubstitutionPermutationNetwork spn, SBoxesWithPath bestApproximation)
		{
			Console.WriteLine($"\nBEST OPTION: round0sboxNum {bestApproximation.sboxNum}\tX {bestApproximation.X}\tY {bestApproximation.Y}\tbias {Math.Abs(0.5 - bestApproximation.probability)}\tSBoxes {SubstitutionPermutationNetwork.GetSboxesMaskBitString(bestApproximation.lastRoundSBoxes)}\tLastRoundInputBits {SubstitutionPermutationNetwork.GetBitString(bestApproximation.lastRoundInputBits)}");

			var targetPartialSubkeys = GenerateTargetPartialSubkeys(SubstitutionPermutationNetwork.GetSboxesNumsFromMask(bestApproximation.lastRoundSBoxes))
										.Select(targetPartialSubkey => (targetPartialSubkey, SubstitutionPermutationNetwork.GetBytesBigEndian(targetPartialSubkey)))
										.ToList();

			var keyProbabilities = targetPartialSubkeys
									.ToDictionary(u => u.Item1, u => 0);
			var hackingSubstitutionPermutationNetwork = new SubstitutionPermutationNetwork(SubstitutionPermutationNetwork.GenerateRandomKey());

			for(int it = 0; it < iterationsCount; it++)
				HackIteration(spn, bestApproximation.sboxNum, bestApproximation.X, bestApproximation.lastRoundInputBits, targetPartialSubkeys, hackingSubstitutionPermutationNetwork, keyProbabilities);

			PrintApproximation(bestApproximation, keyProbabilities);
		}

		private static void HackIteration(SubstitutionPermutationNetwork spn, int round0sboxNum, uint round0x, uint lastRoundInputBits, List<(uint, byte[])> targetPartialSubkeys, SubstitutionPermutationNetwork hackingSubstitutionPermutationNetwork, Dictionary<uint, int> keyProbabilities)
		{
			var plainText = GenerateRandomPlainText();
			var enc = spn.Encrypt(plainText);

			var p_setBitsCount = SubstitutionPermutationNetwork.CountBits(SubstitutionPermutationNetwork.GetUintBigEndian(plainText) & (round0x << ((SubstitutionPermutationNetwork.RoundSBoxesCount - round0sboxNum - 1) * SBox.BitSize)));

			var encUnkeyed = new byte[enc.Length];
			foreach(var (targetPartialSubkey, subkeyBytes) in targetPartialSubkeys)
			{
				for(int i = 0; i < subkeyBytes.Length; i++)
					encUnkeyed[i] = (byte)(enc[i] ^ subkeyBytes[i]);

				hackingSubstitutionPermutationNetwork.DecryptRound(encUnkeyed, null, hackingSubstitutionPermutationNetwork.sboxes[SubstitutionPermutationNetwork.RoundsCount - 1], true, true);

				var u_setBitsCount = SubstitutionPermutationNetwork.CountBits(lastRoundInputBits & SubstitutionPermutationNetwork.GetUintBigEndian(encUnkeyed));

				if((p_setBitsCount + u_setBitsCount) % 2 == 0)
					keyProbabilities[targetPartialSubkey]++;
			}
		}

		private static void PrintApproximation_Fixed(Layer bestLayerApproximation, Dictionary<uint, int> keyProbabilities)
		{
			Console.WriteLine($"ITERATIONS DONE: {iterationsCount}");
			Console.ForegroundColor = ConsoleColor.Blue;
			Console.WriteLine($" {Key.ToHexUpperCase()} : REAL KEY");
			Console.ResetColor();

			var expectedCountBias = Math.Abs(0.5 - bestLayerApproximation.inputProbability) * iterationsCount;

			var keyValuePairs = keyProbabilities.OrderByDescending(kvp => Math.Abs(iterationsCount / 2 - kvp.Value)).ToList();
			var gotCountBias = Math.Abs(iterationsCount / 2 - keyValuePairs[0].Value);

			if(Math.Abs(expectedCountBias - gotCountBias) > expectedCountBias / 2)
			{
				Console.ForegroundColor = ConsoleColor.Red;
				Console.WriteLine($" best {gotCountBias} expected {expectedCountBias}");
				Console.ResetColor(); 
//				return;
			}

			int prevBias = -1;
			var prefix = "";
			foreach(var kvp in keyValuePairs.Take(16))
			{
				var keyBytes = SubstitutionPermutationNetwork.GetBytesBigEndian(kvp.Key);
				var isValidKey = IsValidKey(Key, keyBytes, bestLayerApproximation.ActivatedSboxesNums);

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

		private static void PrintApproximation(SBoxesWithPath bestApproximation, Dictionary<uint, int> keyProbabilities)
		{
			Console.WriteLine($"ITERATIONS DONE: {iterationsCount}");
			Console.ForegroundColor = ConsoleColor.Blue;
			Console.WriteLine($" {Key.ToHexUpperCase()} : REAL KEY");
			Console.ResetColor();

			var expectedCountBias = Math.Abs(0.5 - bestApproximation.probability) * iterationsCount;

			var keyValuePairs = keyProbabilities.OrderByDescending(kvp => Math.Abs(iterationsCount / 2 - kvp.Value)).ToList();
			var gotCountBias = Math.Abs(iterationsCount / 2 - keyValuePairs[0].Value);

			if(Math.Abs(expectedCountBias - gotCountBias) > expectedCountBias / 2)
			{
				Console.ForegroundColor = ConsoleColor.Red;
				Console.WriteLine($" best {gotCountBias} expected {expectedCountBias}");
				Console.ResetColor();
//				return;
			}

			int prevBias = -1;
			var prefix = "";
			foreach(var kvp in keyValuePairs.Take(16))
			{
				var keyBytes = SubstitutionPermutationNetwork.GetBytesBigEndian(kvp.Key);
				var isValidKey = IsValidKey(Key, keyBytes, SubstitutionPermutationNetwork.GetSboxesNumsFromMask(bestApproximation.lastRoundSBoxes));

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

		private static List<uint> GenerateTargetPartialSubkeys(List<int> vulnerableLastRoundSBoxesNums)
		{
			var targetPartialSubkeys = new List<uint> { 0 };

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

		private static bool IsValidKey(byte[] expected, byte[] got, ICollection<int> lastRoundSBoxesNums)
		{
			foreach(var sBoxNum in lastRoundSBoxesNums)
			{
				var byteNum = sBoxNum / 2;
				if(((got[byteNum] ^ expected[byteNum]) & (sBoxNum % 2 == 0 ? 0xF0 : 0x0F)) != 0)
					return false;
			}
			return true;
		}
	}
}
