public final class PBox {
	var outputBitNumbersZeroBased : [Int]
	var invOutputBitNumbersZeroBased : [Int]

	init(_ outputBitNumbers: [Int]){
		outputBitNumbersZeroBased = outputBitNumbers.map { $0 - 1 }
		invOutputBitNumbersZeroBased = [Int](repeating: 0, count: outputBitNumbersZeroBased.count)
		for i in (0..<invOutputBitNumbersZeroBased.count) {
			invOutputBitNumbersZeroBased[outputBitNumbersZeroBased[i]] = i
		}
	}

	func permute(_ input: UInt64) -> UInt64 {
		return permuteInternal(input, outputBitNumbersZeroBased);
	}

	func invPermute(_ input: UInt64) -> UInt64 {
		return permuteInternal(input, invOutputBitNumbersZeroBased);
	}

	func permuteInternal(_ input: UInt64, _ bitNumbers: [Int]) -> UInt64 {
		var result: UInt64 = 0;

		for (i, element) in bitNumbers.enumerated()
		{
			let inputBitValue = (input >> (bitNumbers.count - (i + 1))) & 0x1
			let outputBitValue = inputBitValue << (bitNumbers.count - (element + 1))
			result |= outputBitValue
		}

		return result
	}
}