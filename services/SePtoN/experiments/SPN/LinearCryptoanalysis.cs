using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;

namespace SPN
{
	class LinearCryptoanalysis
	{
		private int maxSBoxesInLastRound;
		private double thresholdBias;

		private readonly SubstitutionPermutationNetwork spn;
		private LinearApproximationTable[][] linearApproximationTables;

		public LinearCryptoanalysis(SubstitutionPermutationNetwork spn)
		{
			this.spn = spn;

			linearApproximationTables = new LinearApproximationTable[spn.sboxes.Length][];
			for(int i = 0; i < spn.sboxes.Length; i++)
			{
				linearApproximationTables[i] = new LinearApproximationTable[spn.sboxes[i].Length];
				for(int j = 0; j < spn.sboxes[i].Length; j++)
					linearApproximationTables[i][j] = new LinearApproximationTable(spn.sboxes[i][j]);
			}
		}

		public IEnumerable<SBoxesWithPath> ChooseBestPathsStartingFromSingleSBoxInRound0(int maxSBoxesInLastRound, double thresholdBias)
		{
			this.maxSBoxesInLastRound = maxSBoxesInLastRound;
			this.thresholdBias = thresholdBias;

			var bests = new List<SBoxesWithPath>();

			for(int round0sboxNum = 0; round0sboxNum < linearApproximationTables[0].Length; round0sboxNum++)
			{
				var linearApproximationTable = linearApproximationTables[0][round0sboxNum].table;

				for(uint X = 0; X < linearApproximationTable.GetLength(0); X++)
				{
					var bestItems = TraceRound(0, round0sboxNum, X);
					bests.AddRange(bestItems);
				}
			}

			bests = bests
				.OrderByDescending(best => Math.Abs(0.5 - best.probability)).ToList();

//			foreach(var best in bests)
//				Console.WriteLine($"round0sboxNum {best.sboxNum}\tX {best.X}\tY {best.Y}\tbias {Math.Abs(0.5 - best.probability)}\tSBoxes {best.lastRoundSBoxes}");

			return bests;
		}

		Dictionary<(int, int, uint), List<SBoxesWithPath>> cache = new Dictionary<(int, int, uint), List<SBoxesWithPath>>();
		private List<SBoxesWithPath> TraceRound(int roundNum, int sboxNum, uint X)
		{
			var cacheKey = (roundNum, sboxNum, X);
			if(cache.ContainsKey(cacheKey))
				return cache[cacheKey];

			var bestApproximations = ChooseBestApproximations(linearApproximationTables[roundNum][sboxNum].table, X);

			var activatedOutputSBoxes = new List<SBoxesWithPath>();

			foreach(var approximation in bestApproximations)
			{
				var probability = approximation.probability;
//				if(roundNum <= 0)
//					Console.WriteLine($"{new string(' ', roundNum)}round {roundNum}, SBox {sboxNum}: X {Convert.ToString(X, 2)}, Y {Convert.ToString(approximation.Y, 2)}: probability {probability}");

				var outputBits = PermuteY(approximation.Y, sboxNum);

				if(roundNum >= SubstitutionPermutationNetwork.RoundsCount - 1 - 1)
				{
					var sBoxes = outputBits.Aggregate(0u, (seed, kvp) => seed | SubstitutionPermutationNetwork.GetSBoxMaskWithNthBit(kvp.Key));
					var bits = outputBits.Aggregate(0u, (seed, kvp) => seed | SubstitutionPermutationNetwork.GetBitMaskWithNthSboxAndMthBit(kvp.Key, kvp.Value));
					var sBoxesWithPath = new SBoxesWithPath(sBoxes, bits, roundNum, sboxNum, approximation.X, approximation.Y, approximation.probability);
					activatedOutputSBoxes.Add(sBoxesWithPath);
					continue;
				}

				var nextRoundsVariants = new List<List<SBoxesWithPath>>();
				foreach(var permutationOutput in outputBits)
				{
					var newSboxNum = permutationOutput.Key;
					var newBitNum = permutationOutput.Value;

					nextRoundsVariants.Add(TraceRound(roundNum + 1, newSboxNum, SBox.GetUintWithNthBit(newBitNum)));
				}

				activatedOutputSBoxes.AddRange(Flattern(nextRoundsVariants, roundNum, sboxNum, approximation));
			}

			cache[cacheKey] = activatedOutputSBoxes;
			return activatedOutputSBoxes;
		}

		private List<SBoxesWithPath> Flattern(List<List<SBoxesWithPath>> children, int roundNum, int sboxNum, (uint X, uint Y, double probability) approximation)
		{
			var result = new List<SBoxesWithPath>();

			var firstChildVariants = children[0];
			List<SBoxesWithPath> fromOtherChildrenResult = null;
			if(children.Count > 1)
				fromOtherChildrenResult = Flattern(children.Skip(1).ToList(), roundNum, sboxNum, approximation);

			foreach(var sBoxWithPath in firstChildVariants)
			{
				SBoxesWithPath resultSBoxWithPath;
				if(fromOtherChildrenResult != null)
					foreach(var fromOtherChildrenSBox in fromOtherChildrenResult)
					{
						resultSBoxWithPath = new SBoxesWithPath(sBoxWithPath, fromOtherChildrenSBox, roundNum, sboxNum, approximation.X, approximation.Y);
						result.Add(resultSBoxWithPath);
					}
				else
				{
					resultSBoxWithPath = new SBoxesWithPath(sBoxWithPath, null, roundNum, sboxNum, approximation.X, approximation.Y, approximation.probability);
					result.Add(resultSBoxWithPath);
				}
			}

			result = result
				.Where(sBoxesWithPath => SubstitutionPermutationNetwork.CountBitsInSboxesMask(sBoxesWithPath.lastRoundSBoxes) <= maxSBoxesInLastRound)
				.Where(sBoxesWithPath => Math.Abs(sBoxesWithPath.probability - 0.5) > thresholdBias)
				.ToList();

			return result;
		}

		private List<KeyValuePair<int, int>> PermuteY(uint Y, int sboxNum)
		{
			var outputBits = Enumerable.Range(0, SBox.BitSize)
				.Where(bitNum => SBox.IsNthBitSet(Y, bitNum))
				.Select(bitNum => spn.pbox.PermuteBit(sboxNum * SBox.BitSize + bitNum))
				.Select(permutedBitNum => new KeyValuePair<int, int>(permutedBitNum / SBox.BitSize, permutedBitNum % SBox.BitSize))
				.ToList();
			return outputBits;
		}

		private List<(uint X, uint Y, double probability)> ChooseBestApproximations(double[,] linearApproximationTable, uint X)
		{
			var result = new List<(uint, uint, double)>();

			//NOTE в левом верхнем углу всегда максимум, пропускаем первый столбец
			for(uint y = 1; y < linearApproximationTable.GetLength(1); y++)
			{
				var probability = linearApproximationTable[X, y];
				if(Math.Abs(probability) != 1 / 2.0d)
					result.Add((X, y, probability));
			}

			return result;
		}
	}

	class SBoxesWithPath
	{
		public uint lastRoundSBoxes;
		public uint lastRoundBits;

		//NOTE для листового узла
		public readonly int roundNum;
		public readonly int sboxNum;
		public readonly uint X;
		public readonly uint Y;
		public readonly double probability;

		public List<SBoxesWithPath> children = new List<SBoxesWithPath>(); //внешний список содержит себе биты, на которые разветвился SBox. А внутренний - варианты резолвинга этого разветвления

		public SBoxesWithPath(SBoxesWithPath firstChild, SBoxesWithPath fromOtherChildrenSBox, int roundNum, int sboxNum, uint X, uint Y, double? probability = null)
		{
			this.roundNum = roundNum;
			this.sboxNum = sboxNum;
			this.X = X;
			this.Y = Y;

			this.lastRoundSBoxes = firstChild.lastRoundSBoxes;
			this.lastRoundBits = firstChild.lastRoundBits;

			children.Add(firstChild);

			if(probability != null)
				this.probability = 1 / 2.0d + 2 * (probability.Value - 1 / 2.0d) * (firstChild.probability - 1 / 2.0d);

			if(fromOtherChildrenSBox != null)
			{
				lastRoundSBoxes |= fromOtherChildrenSBox.lastRoundSBoxes;
				lastRoundBits |= fromOtherChildrenSBox.lastRoundBits;
				children.AddRange(fromOtherChildrenSBox.children);

				this.probability = 1 / 2.0d + 2 * (firstChild.probability - 1 / 2.0d) * (fromOtherChildrenSBox.probability - 1 / 2.0d);
			}

			
		}

		public SBoxesWithPath(uint lastRoundSBoxes, uint lastRoundBits, int roundNum, int sboxNum, uint X, uint Y, double probability)
		{
			this.lastRoundSBoxes = lastRoundSBoxes;
			this.lastRoundBits = lastRoundBits;
			this.roundNum = roundNum;
			this.sboxNum = sboxNum;
			this.X = X;
			this.Y = Y;
			this.probability = probability;
		}
	}
}
