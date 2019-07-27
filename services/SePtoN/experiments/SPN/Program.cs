﻿using System;
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

			var plainTextString = new string('X', SubstitutionPermutationNetwork.BlockSizeBytes);
			var plainText = Encoding.ASCII.GetBytes(plainTextString);
			var encryptedBytes = spn.Encrypt(plainText);
			var decryptedBytes = spn.Decrypt(encryptedBytes);
			var decryptedString = Encoding.ASCII.GetString(decryptedBytes);
			Console.WriteLine($"{plainTextString} -> {encryptedBytes.ToHexUpperCase()} -> {decryptedString}");

//			HackCipher(spn);
			HackCipher_Fixed(spn);
		}

		const double thresholdBias = 0.004;
		const int maxSBoxesInLastRound = 2;
		const int maxSBoxesInRound = 2 * maxSBoxesInLastRound;

		const int iterationsCount = 65536;


		private static void HackCipher_Fixed(SubstitutionPermutationNetwork spn)
		{
			var linearCryptoanalysis = new LinearCryptoanalysis(spn);

			var bestLayerApproximations = linearCryptoanalysis.ChooseBestPathsStartingFromSingleSBoxInRound0(maxSBoxesInLastRound, maxSBoxesInRound, thresholdBias).ToList();
			Console.WriteLine($"Total approximations: {bestLayerApproximations.Count}");

			var plains = Enumerable.Range(0, iterationsCount).Select(i => GenerateRandomPlainText()).ToArray();
			var encs = plains.Select(plain => spn.Encrypt(plain)).ToArray();

			var solutions = new List<Solution>();
			foreach(var approximationsGroup in bestLayerApproximations
													.Where(layer => layer.round0sboxNum % 2 == 0 && layer.round0x == 0x8)
													.GroupBy(layer => layer.ActivatedSboxesNums.Aggregate("", (s, num) => s + num))
													.OrderBy(group => group.Key))
			{
				foreach(var approximation in approximationsGroup.Distinct().OrderByDescending(layer => layer.inputProbability.Bias()).Take(3))
					solutions.AddRange(HackApproximation_Fixed(plains, encs, approximation));
			}

			PrintSolution(solutions);
		}

		private static void PrintSolution(List<Solution> solutions)
		{
			var keyCandidate = new string(' ',2 * Key.Length).ToCharArray();
			var lines = new List<char[]>();

			foreach(var sboxNumGroup in solutions.GroupBy(solution => solution.SBoxNum).OrderBy(grouping => grouping.Key))
			{
				var hexNum = sboxNumGroup.Key;

				var candidates = sboxNumGroup
					.GroupBy(solution => solution.Byte)
					.Select(g =>
						{
							var sumScores = g.Aggregate(0.0, (s, solution) => s + solution.score);
							return (g, sumScores);
						})
					.OrderByDescending(tuple => tuple.sumScores)
					.ToList();


				for(int i = 0; i < candidates.Count; i++)
				{
					var group = candidates[i];
					if(lines.Count == i)
						lines.Add(keyCandidate.ToArray());

					lines[i][hexNum] = group.g.Key.ToHex4B();

//					Console.WriteLine($"{sboxNumGroup.Key}: {group.g.Key.ToHex4B()} = {group.sumScores}");
				}
			}

			foreach(var line in lines)
			{
				Console.WriteLine(new string(line));
			}
		}


		private static List<Solution> HackApproximation_Fixed(byte[][] plains, byte[][] encs, Layer bestLayerApproximation)
		{
			Console.WriteLine($"\nBEST OPTION: round0sboxNum {bestLayerApproximation.round0sboxNum}\tround0x {bestLayerApproximation.round0x}\tround0y {bestLayerApproximation.round0y}\tbias {bestLayerApproximation.inputProbability.Bias()}\toutSBoxes {string.Join(",", bestLayerApproximation.ActivatedSboxesNums)}\tLastRoundInputBits {SubstitutionPermutationNetwork.GetBitString(bestLayerApproximation.inputBits)}");

			var targetPartialSubkeys = GenerateTargetPartialSubkeys(bestLayerApproximation.ActivatedSboxesNums)
				.Select(targetPartialSubkey => (targetPartialSubkey, SubstitutionPermutationNetwork.GetBytesBigEndian(targetPartialSubkey)))
				.ToList();

			var keyProbabilities = targetPartialSubkeys
				.ToDictionary(u => u.Item1, u => 0);
			var hackingSubstitutionPermutationNetwork = new SubstitutionPermutationNetwork(SubstitutionPermutationNetwork.GenerateRandomKey());

			for(int i = 0; i < iterationsCount; i++)
			{
				if(i > 0 && i % (iterationsCount / 4) == 0)
				{
					Console.WriteLine($" done {i} iterations of {iterationsCount}");
				}

				var plain = plains[i];
				var enc = encs[i];

				HackIteration(plain, enc, bestLayerApproximation.round0sboxNum, bestLayerApproximation.round0x, bestLayerApproximation.inputBits, targetPartialSubkeys, hackingSubstitutionPermutationNetwork, keyProbabilities);
			}

			return PrintApproximation_Fixed(bestLayerApproximation, keyProbabilities);
		}

		private static void HackIteration(byte[] plain, byte[] enc, int round0sboxNum, ulong round0x, ulong lastRoundInputBits, List<(ulong, byte[])> targetPartialSubkeys, SubstitutionPermutationNetwork hackingSubstitutionPermutationNetwork, Dictionary<ulong, int> keyProbabilities)
		{
			var p_setBitsCount = SubstitutionPermutationNetwork.CountBits(SubstitutionPermutationNetwork.GetUlongBigEndian(plain) & (round0x << ((SubstitutionPermutationNetwork.RoundSBoxesCount - round0sboxNum - 1) * SBox.BitSize)));

			var encUnkeyed = new byte[enc.Length];
			foreach(var (targetPartialSubkey, subkeyBytes) in targetPartialSubkeys)
			{
				for(int i = 0; i < subkeyBytes.Length; i++)
					encUnkeyed[i] = (byte)(enc[i] ^ subkeyBytes[i]);

				hackingSubstitutionPermutationNetwork.DecryptRound(encUnkeyed, null, hackingSubstitutionPermutationNetwork.sboxes[SubstitutionPermutationNetwork.RoundsCount - 1], true, true);

				var u_setBitsCount = SubstitutionPermutationNetwork.CountBits(lastRoundInputBits & SubstitutionPermutationNetwork.GetUlongBigEndian(encUnkeyed));

				if((p_setBitsCount + u_setBitsCount) % 2 == 0)
					keyProbabilities[targetPartialSubkey]++;
			}
		}

		private static List<Solution> PrintApproximation_Fixed(Layer bestLayerApproximation, Dictionary<ulong, int> keyProbabilities)
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

			var solutions = new List<Solution>();
			int groupNum = 0;
			int prevBias = -1;
			var prefix = "";
			foreach(var kvp in keyValuePairs.Take(16))
			{
				var keyBytes = SubstitutionPermutationNetwork.GetBytesBigEndian(kvp.Key);
				var isValidKey = IsValidKey(Key, keyBytes, bestLayerApproximation.ActivatedSboxesNums);

				var bias = Math.Abs(iterationsCount / 2 - kvp.Value);

				if(bias != prevBias && prevBias != -1)
				{
					prefix = prefix == " " ? "" : " ";
					groupNum++;
				}

				if(isValidKey)
					Console.ForegroundColor = ConsoleColor.Green;
				Console.WriteLine($"{prefix}{keyBytes.ToHexUpperCase()} : {bias}");
				Console.ResetColor();
				prevBias = bias;

				if(groupNum < 3)
				{
					solutions.AddRange(GetSolutions(keyBytes, bestLayerApproximation.ActivatedSboxesNums, groupNum));
				}
			}

			return solutions;
		}

		private static List<ulong> GenerateTargetPartialSubkeys(List<int> vulnerableLastRoundSBoxesNums)
		{
			var targetPartialSubkeys = new List<ulong> { 0u };

			while(vulnerableLastRoundSBoxesNums.Count > 0)
			{
				var sBoxNum = vulnerableLastRoundSBoxesNums.Last();

				var newTargetPartialSubkeys = new List<ulong>();
				foreach(var targetPartialSubkey in targetPartialSubkeys)
					for(ulong v = 0; v < 1u << SBox.BitSize; v++)
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

		private static IEnumerable<Solution> GetSolutions(byte[] got, List<int> activatedSboxesNums, int groupNum)
		{
			foreach(var sBoxNum in activatedSboxesNums)
			{
				var byteNum = sBoxNum / 2;
				var b = (byte)(got[byteNum] >> (sBoxNum % 2 == 0 ? 4 : 0));
				yield return new Solution
				{
					Byte = b,
					SBoxNum = sBoxNum,
					score = 1 / Math.Pow(2, groupNum)
				};
			}
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

	class Solution
	{
		public int SBoxNum;
		public byte Byte;
		public double score;
	}
}