using System;
using System.Collections.Generic;
using System.Collections.Immutable;
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

		

		public IEnumerable<Layer> ChooseBestPathsStartingFromSingleSBoxInRound0(int maxSBoxesInLastRound, int maxSBoxesInRound, double thresholdBias)
		{
			var round = 0;
			var roundLayerCandidates = new List<Layer>();
			for(int round0sboxNum = 0; round0sboxNum < linearApproximationTables[0].Length; round0sboxNum++)
			{
				var linearApproximationTable = linearApproximationTables[0][round0sboxNum].table;

				for(uint X = 0; X < linearApproximationTable.GetLength(0); X++)
				{
					var bestApproximations = ChooseBestApproximations(linearApproximationTable, X);

					foreach(var approximation in bestApproximations)
					{
						roundLayerCandidates.Add(new Layer
						{
							roundNum = 0,
							activatedSBoxes = new[] { (round0sboxNum, approximation.X, approximation.Y, approximation.probability) },
							inputProbability = 1,
							outputProbability = approximation.probability
						});
					}
				}
			}

			while(round < SubstitutionPermutationNetwork.RoundsCount - 1)
			{
				var roundThreshold = thresholdBias * Math.Pow(2, SubstitutionPermutationNetwork.RoundsCount - round - 2);

				Console.WriteLine($"Tracing round {round}, roundLayerCandidates {roundLayerCandidates.Count}");

				var newRoundLayerCandidates = new List<Layer>();
				for(var i = 0; i < roundLayerCandidates.Count; i++)
				{
					var fractionSize = roundLayerCandidates.Count / 4;
					if(i > 0 && fractionSize > 0 && i % fractionSize == 0)
					{
						double minBias = newRoundLayerCandidates.Count > 0 ? newRoundLayerCandidates.Min(layer => layer.inputProbability.Bias()) : 0;
						double maxBias = newRoundLayerCandidates.Count > 0 ? newRoundLayerCandidates.Max(layer => layer.inputProbability.Bias()) : 0;
						Console.WriteLine($" done {i + 1}\t=> newRoundLayerCandidates {newRoundLayerCandidates.Count},\tminBias {minBias} maxBias {maxBias} threshold {roundThreshold}");
					}

					newRoundLayerCandidates.AddRange(TraceRoundVariants(roundLayerCandidates[i], maxSBoxesInRound, roundThreshold));
				}

				roundLayerCandidates = newRoundLayerCandidates;
				round++;
			}

			return roundLayerCandidates
				.Where(layer => layer.activatedSBoxes.Length <= maxSBoxesInLastRound)
				.OrderByDescending(layer => layer.inputProbability.Bias())
				.ThenBy(layer => layer.activatedSBoxes.Length);
		}

		private IEnumerable<Layer> TraceRoundVariants(Layer prevLayer, int maxSBoxesInRound, double roundThreshold)
		{
			var roundNum = prevLayer.roundNum + 1;
			var inputProbability = prevLayer.outputProbability;

			var sboxesWithInputs = prevLayer.activatedSBoxes
				.SelectMany(activatedSBox => PermuteY(activatedSBox.Y, activatedSBox.sboxNum))
				.GroupBy(kvp => kvp.Key)
				.Select(grouping =>
				{
					var newSboxNum = grouping.Key;
					uint newX = 0;
					foreach(var kvp in grouping)
					{
						var newBitNum = kvp.Value;
						newX |= SBox.GetUintWithNthBit(newBitNum);
					}

					return (newSboxNum, newX);
				})
				.ToList();

			if(sboxesWithInputs.Count > maxSBoxesInRound)
				return ImmutableArray<Layer>.Empty;

			return GenerateLayers(prevLayer, roundNum, inputProbability, sboxesWithInputs, roundThreshold);
		}

		private IEnumerable<Layer> GenerateLayers(Layer prevLayer, int roundNum, double inputProbability, List<(int sboxNum, uint X)> sboxesWithInputs, double roundThreshold)
		{
			var activatedSBoxesVariants = sboxesWithInputs.Select(sboxWithInput =>
			{
				var bestApproximations = ChooseBestApproximations(linearApproximationTables[roundNum][sboxWithInput.sboxNum].table, sboxWithInput.X);
				return bestApproximations.Select(bestApproximation => (sboxWithInput.sboxNum, bestApproximation.X, bestApproximation.Y, bestApproximation.probability));
			}).ToList().CrossJoin();

			return activatedSBoxesVariants.Select(variant =>
			{
				var activatedSBoxes = variant.ToArray();

				double outputProbability = inputProbability;
				foreach(var activatedSBox in activatedSBoxes)
					outputProbability = 1 / 2.0d + 2 * (outputProbability - 1 / 2.0d) * (activatedSBox.probability - 1 / 2.0d);

				if(Math.Abs(0.5 - outputProbability) < roundThreshold)
					return null;

				return new Layer
				{
					roundNum = roundNum,
					inputProbability = inputProbability,
					outputProbability = outputProbability,
					prevLayer = prevLayer,
					activatedSBoxes = activatedSBoxes
				};
			}).Where(layer => layer != null);
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

	public class Layer
	{
		public int roundNum;
		public Layer prevLayer;
		public double inputProbability;
		public double outputProbability;
		public (int sboxNum, uint X, uint Y, double probability)[] activatedSBoxes;

		public int round0sboxNum
		{
			get
			{
				var prev = prevLayer;
				while(prev.prevLayer != null)
					prev = prev.prevLayer;

				return prev.activatedSBoxes.Single().sboxNum;
			}
		}

		public uint round0x
		{
			get
			{
				var prev = prevLayer;
				while(prev.prevLayer != null)
					prev = prev.prevLayer;

				return prev.activatedSBoxes.Single().X;
			}
		}

		public uint round0y
		{
			get
			{
				var prev = prevLayer;
				while(prev.prevLayer != null)
					prev = prev.prevLayer;

				return prev.activatedSBoxes.Single().Y;
			}
		}

		public uint inputBits
		{
			get
			{
				uint result = 0;
				foreach(var activatedSBox in activatedSBoxes)
					result |= SubstitutionPermutationNetwork.GetBitMask(activatedSBox.sboxNum, activatedSBox.X);
				return result;
			}
		}

		public List<int> ActivatedSboxesNums => activatedSBoxes.Select(tuple => tuple.sboxNum).OrderBy(i => i).ToList();
	}
}
