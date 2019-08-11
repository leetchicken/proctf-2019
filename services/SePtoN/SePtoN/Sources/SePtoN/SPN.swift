import Foundation
#if os(Linux) || os(FreeBSD)
  import Glibc
#endif

public final class SPN {

	static let roundsCount = 5

	static var roundSBoxesCount: Int { return SBoxes.count }
	static var blockSizeBytes: Int { return (roundSBoxesCount * SBox.bitSize) / 8 }
	static var keySizeBytes: Int { return (roundSBoxesCount * SBox.bitSize) / 8 }

	static let SBox1: [UInt8] = [0x4, 0xA, 0x9, 0x2, 0xD, 0x8, 0x0, 0xE, 0x6, 0xB, 0x1, 0xC, 0x7, 0xF, 0x5, 0x3]
	static let SBox2: [UInt8] = [0xE, 0xB, 0x4, 0xC, 0x6, 0xD, 0xF, 0xA, 0x2, 0x3, 0x8, 0x1, 0x0, 0x7, 0x5, 0x9];
	static let SBox3: [UInt8] = [0x5, 0x8, 0x1, 0xD, 0xA, 0x3, 0x4, 0x2, 0xE, 0xF, 0xC, 0x7, 0x6, 0x0, 0x9, 0xB];
	static let SBox4: [UInt8] = [0x7, 0xD, 0xA, 0x1, 0x0, 0x8, 0x9, 0xF, 0xE, 0x4, 0x6, 0xC, 0xB, 0x2, 0x5, 0x3];
	static let SBox5: [UInt8] = [0x6, 0xC, 0x7, 0x1, 0x5, 0xF, 0xD, 0x8, 0x4, 0xA, 0x9, 0xE, 0x0, 0x3, 0xB, 0x2];
	static let SBox6: [UInt8] = [0x4, 0xB, 0xA, 0x0, 0x7, 0x2, 0x1, 0xD, 0x3, 0x6, 0x8, 0x5, 0x9, 0xC, 0xF, 0xE];
	static let SBox7: [UInt8] = [0xD, 0xB, 0x4, 0x1, 0x3, 0xF, 0x5, 0x9, 0x0, 0xA, 0xE, 0x7, 0x6, 0x8, 0x2, 0xC];
	static let SBox8: [UInt8] = [0x1, 0xF, 0xD, 0x0, 0x5, 0x7, 0xA, 0x4, 0x9, 0x2, 0x3, 0xE, 0x6, 0xB, 0x8, 0xC];
	static let SBox9: [UInt8] = [ 0xC, 0x4, 0x6, 0x2, 0xA, 0x5, 0xB, 0x9, 0xE, 0x8, 0xD, 0x7, 0x0, 0x3, 0xF, 0x1 ];
	static let SBox10: [UInt8] = [ 0x6, 0x8, 0x2, 0x3, 0x9, 0xA, 0x5, 0xC, 0x1, 0xE, 0x4, 0x7, 0xB, 0xD, 0x0, 0xF ];
	static let SBox11: [UInt8] = [ 0xB, 0x3, 0x5, 0x8, 0x2, 0xF, 0xA, 0xD, 0xE, 0x1, 0x7, 0x4, 0xC, 0x9, 0x6, 0x0 ];
	static let SBox12: [UInt8] = [ 0xC, 0x8, 0x2, 0x1, 0xD, 0x4, 0xF, 0x6, 0x7, 0x0, 0xA, 0x5, 0x3, 0xE, 0x9, 0xB ];
	static let SBox13: [UInt8] = [ 0x7, 0xF, 0x5, 0xA, 0x8, 0x1, 0x6, 0xD, 0x0, 0x9, 0x3, 0xE, 0xB, 0x4, 0x2, 0xC ];
	static let SBox14: [UInt8] = [ 0x5, 0xD, 0xF, 0x6, 0x9, 0x2, 0xC, 0xA, 0xB, 0x7, 0x8, 0x1, 0x4, 0x3, 0xE, 0x0 ];
	static let SBox15: [UInt8] = [ 0x8, 0xE, 0x2, 0x5, 0x6, 0x9, 0x1, 0xC, 0xF, 0x4, 0xB, 0x0, 0xD, 0xA, 0x3, 0x7 ];
	static let SBox16: [UInt8] = [ 0x1, 0x7, 0xE, 0xD, 0x0, 0x5, 0x8, 0x3, 0x4, 0xF, 0xA, 0x6, 0x9, 0xC, 0xB, 0x2 ];

	static let SBoxes: [[UInt8]] = [SBox1, SBox2, SBox3, SBox4, SBox5, SBox6, SBox7, SBox8, SBox9, SBox10, SBox11, SBox12, SBox13, SBox14, SBox15, SBox16]
	static let pBoxOutput: [Int] = [38, 4, 15, 46, 11, 16, 33, 1, 35, 64, 51, 45, 50, 55, 27, 57, 47, 52, 43, 12, 7, 40, 42, 53, 29, 10, 56, 60, 36, 20, 58, 24, 39, 37, 26, 3, 32, 17, 22, 28, 30, 23, 63, 49, 14, 62, 19, 25, 21, 5, 9, 6, 8, 34, 18, 13, 31, 61, 44, 2, 48, 41, 54, 59]	

	
	static func generateIV() -> [UInt8] {
		let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: blockSizeBytes)
	  #if os(Linux) || os(FreeBSD)
		let fd = open("/dev/urandom", O_RDONLY)
        defer {
            close(fd)
        }
        let _ = read(fd, buffer, MemoryLayout<UInt8>.size * blockSizeBytes)
      #else
        arc4random_buf(buffer, blockSizeBytes)
      #endif
        defer {
            buffer.deinitialize(count: blockSizeBytes)
            buffer.deallocate()
        }

        return Array(Data(bytesNoCopy: buffer, count: blockSizeBytes, deallocator: .none))
	}

	static func CalcMasterKey(_ keyMaterial: [UInt8]) throws -> [UInt8] {
		if (keyMaterial.count == 0 || keyMaterial.count % SPN.blockSizeBytes != 0) {
			throw SPNError.unpaddedInput
		}

		var i = SPN.blockSizeBytes
		var masterKey = Array(keyMaterial[0..<i])
        while (i < keyMaterial.count) {
        	masterKey = SPN.xorBlock(masterKey, Array(keyMaterial[i..<(i+SPN.blockSizeBytes)]))
        	i += SPN.blockSizeBytes
        }
        return masterKey
    }

	static func generateSBoxes() -> [[SBox]] {
		return (0..<roundsCount).map { _ in
			(0..<roundSBoxesCount).map { SBox(SBoxes[$0])}
		}
	}

	static func keyShedule(_ masterKey: [UInt8]) -> [[UInt8]] {
		return (0...roundsCount).map { _ in masterKey }
	}



	init(_ masterKey: [UInt8]) {
		self.masterKey = masterKey
		subkeys = SPN.keyShedule(masterKey)
		sboxes = SPN.generateSBoxes()
		pbox = PBox(SPN.pBoxOutput)
	}

	func encryptCBC(_ data: [UInt8], _ iv: [UInt8]) throws -> [UInt8] {
		if (data.count == 0 || data.count % SPN.blockSizeBytes != 0 || iv.count % SPN.blockSizeBytes != 0) {
			throw SPNError.unpaddedInput
		}

		var result = [UInt8]()
		var i = 0
		result.reserveCapacity(iv.count + data.count)		
		result += iv
		
		var prevC = iv
		while(i < data.count){
			let p = Array(data[i..<(i+SPN.blockSizeBytes)])
			let c = encryptBlock(SPN.xorBlock(p, prevC))
			result += c
			prevC = c
			i += SPN.blockSizeBytes
		}

		return result
	}

	func encryptBlock(_ block: [UInt8]) -> [UInt8] {
		var result = block
		for roundNum in (0..<SPN.roundsCount) {
			result = encryptRound(result, subkeys[roundNum], sboxes[roundNum], roundNum == SPN.roundsCount - 1)
		}

		result = xorWithLastSubKey(result);
		return result;
	}

	func encryptRound(_ input: [UInt8], _ subKey: [UInt8], _ sboxes: [SBox], _ skipPermutation: Bool) -> [UInt8] {
		var result = zip(input, subKey).map { $0.0 ^ $0.1 }

		for i in (0..<result.count) {
			var leftPart = UInt8(result[i] >> 4)
			var rightPart = UInt8(result[i] & 0x0F)

			leftPart = sboxes[i * 2].substitute(leftPart)
			rightPart = sboxes[i * 2 + 1].substitute(rightPart)

			result[i] = (leftPart << 4) | rightPart
		}

		if(!skipPermutation) {
			let pboxInput = result.reduce(UInt64(0), { x, y in 
				(x << 8) | UInt64(y)
			})
			var pboxOutput = pbox.permute(pboxInput)

			for i in (0..<result.count) {
				result[result.count - i - 1] = UInt8(pboxOutput & 0xFF)
				pboxOutput >>= 8
			}			
		}

		return result
	}

	func decryptCBC(_ data: [UInt8]) throws -> [UInt8] {
		if (data.count == 0 || data.count % SPN.blockSizeBytes != 0) {
			throw SPNError.unpaddedInput
		}

		var result = [UInt8]()
		var i = SPN.blockSizeBytes
		result.reserveCapacity(data.count - i)		

		var prevC = Array(data[0..<i])
		while(i < data.count){
			let c = Array(data[i..<(i+SPN.blockSizeBytes)])
			let p = SPN.xorBlock(decryptBlock(c), prevC)
			result += p
			prevC = c
			i += SPN.blockSizeBytes
		}

		return result
	}

	func decryptBlock(_ block: [UInt8]) -> [UInt8] {
		var result = block		
		result = xorWithLastSubKey(result);

		for roundNum in (0..<SPN.roundsCount).reversed() {			
			result = decryptRound(result, subkeys[roundNum], sboxes[roundNum], roundNum == SPN.roundsCount - 1)
		}		
		return result;
	}

	func decryptRound(_ input: [UInt8], _ subKey: [UInt8], _ sboxes: [SBox], _ skipPermutation: Bool) -> [UInt8] {
		var result = input
		if(!skipPermutation) {
			let pboxInput = result.reduce(UInt64(0), { x, y in 
				(x << 8) | UInt64(y)
			})
			var pboxOutput = pbox.invPermute(pboxInput)

			for i in (0..<result.count) {
				result[result.count - i - 1] = UInt8(pboxOutput & 0xFF)
				pboxOutput >>= 8
			}			
		}

		for i in (0..<result.count) {
			var leftPart = UInt8(result[i] >> 4)
			var rightPart = UInt8(result[i] & 0x0F)

			leftPart = sboxes[i * 2].invSubstitute(leftPart)
			rightPart = sboxes[i * 2 + 1].invSubstitute(rightPart)

			result[i] = (leftPart << 4) | rightPart
		}		

		result = zip(result, subKey).map { $0.0 ^ $0.1}	

		return result
	}

	func xorWithLastSubKey(_ input: [UInt8]) -> [UInt8] {
		var result = input
		for i in (0..<input.count) {
			result[i] ^= subkeys[SPN.roundsCount][i]
		}
		return result
	}

	static func xorBlock(_ lhs: [UInt8], _ rhs: [UInt8]) -> [UInt8] {
		return zip(lhs, rhs).map { $0.0 ^ $0.1}
	}

	let sboxes: [[SBox]]
	let pbox: PBox
	let subkeys: [[UInt8]];
	let masterKey: [UInt8]
}

enum SPNError: Error {
    case unpaddedInput
}