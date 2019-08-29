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
		if (keyMaterial.count < SPN.blockSizeBytes) {
			throw SPNError.notEnoughKeyMaterial
		}

		var i = SPN.blockSizeBytes
		var masterKey = Array(keyMaterial[0..<i])
        while (i+SPN.blockSizeBytes <= keyMaterial.count) {
        	var k = Array(keyMaterial[i..<(i+SPN.blockSizeBytes)])
        	SPN.xorBlock(&masterKey, &k)
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

	func encryptWithPadding(_ data: [UInt8], _ iv: [UInt8]) throws -> [UInt8] {
		return try encryptCBC(pad(data), iv)
	}

	func pad(_ data: [UInt8]) -> [UInt8] {
		let padLength = SPN.blockSizeBytes - data.count % SPN.blockSizeBytes
		
		var newData = [UInt8]()
		var i = 0
		newData.reserveCapacity(data.count + padLength)
		newData += data

		while(i < padLength) {
			newData.append(UInt8(padLength))
			i+=1			
		}

		return newData;
	}


	func encryptCBC(_ data: [UInt8], _ iv: [UInt8]) throws -> [UInt8] {
// var start = ProcessInfo.processInfo.systemUptime

		if (data.count == 0 || data.count % SPN.blockSizeBytes != 0 || iv.count == 0 || iv.count % SPN.blockSizeBytes != 0) {
			print("data \(data.count) iv \(iv.count)")
			throw SPNError.unpaddedInput
		}

		var result = [UInt8]()
		var i = 0
		result.reserveCapacity(iv.count + data.count)		
		result += iv
		
		var prevC = iv
		while(i < data.count){


			var p = Array(data[i..<(i+SPN.blockSizeBytes)])
			SPN.xorBlock(&p, &prevC)
// var start = ProcessInfo.processInfo.systemUptime
			let c = encryptBlock(p)
// print("----Encrypted block in \(ProcessInfo.processInfo.systemUptime - start) sec")
			result += c
			prevC = c
			i += SPN.blockSizeBytes

			

		}
// print("--------EncryptedCBC in \(ProcessInfo.processInfo.systemUptime - start) sec")
		return result
	}

	func encryptBlock(_ block: [UInt8]) -> [UInt8] {
		var result = block

		for roundNum in (0..<SPN.roundsCount) {
// var start = ProcessInfo.processInfo.systemUptime
			result = encryptRound(&result, &subkeys[roundNum], &sboxes[roundNum], roundNum == SPN.roundsCount - 1)
// print("--------Encrypted round in \(ProcessInfo.processInfo.systemUptime - start) sec")
		}


		result = xorWithLastSubKey(result);
		return result;
	}

	func encryptRound(_ input: inout [UInt8], _ subKey: inout [UInt8], _ sboxes: inout [SBox], _ skipPermutation: Bool) -> [UInt8] {
		var result = Array(input)

// var start = ProcessInfo.processInfo.systemUptime		
		SPN.xorBlock(&result, &subKey)
// print("Xor in \(ProcessInfo.processInfo.systemUptime - start) sec")
// start = ProcessInfo.processInfo.systemUptime

		for i in (0..<result.count) {


			var leftPart = UInt8(result[i] >> 4)
			var rightPart = UInt8(result[i] & 0x0F)


			leftPart = sboxes[i * 2].substitute(leftPart)
			rightPart = sboxes[i * 2 + 1].substitute(rightPart)


			result[i] = (leftPart << 4) | rightPart

		}
// print("SBoxes in \(ProcessInfo.processInfo.systemUptime - start) sec")
// start = ProcessInfo.processInfo.systemUptime

		if(!skipPermutation) {
			let pboxInput = assembleUInt64(result)
// print("PBoxIn in \(ProcessInfo.processInfo.systemUptime - start) sec")
// start = ProcessInfo.processInfo.systemUptime

			var pboxOutput = pbox.permute(pboxInput)

// print("PBoxPermute in \(ProcessInfo.processInfo.systemUptime - start) sec")
// start = ProcessInfo.processInfo.systemUptime

			for i in (0..<result.count) {
				result[result.count - i - 1] = UInt8(pboxOutput & 0xFF)
				pboxOutput >>= 8
			}			
// print("other in \(ProcessInfo.processInfo.systemUptime - start) sec")

		}
// print("Done in \(ProcessInfo.processInfo.systemUptime - start) sec")
// print("====")

		return result
	}

	func assembleUInt64(_ data: [UInt8]) -> UInt64 {
		var result: UInt64 = 0
		for c in data {
			result = (result << 8) | UInt64(c)
		}
		return result
	}

	func decryptWithPadding(_ data: [UInt8]) throws -> [UInt8] {
		return try unPad(decryptCBC(data))
	}

	
	func unPad(_ data: [UInt8]) throws -> [UInt8] {
		if (data.count == 0 || data.count % SPN.blockSizeBytes != 0) {
			throw SPNError.unpaddedInput
		}

		let padLength = Int(data[data.count - 1])
		if(padLength == 0 || padLength > SPN.blockSizeBytes) {
			throw SPNError.unpaddedInput
		}

		var i = data.count - 2
		while(i >= data.count - padLength) {
			if(data[i] != padLength) {
				throw SPNError.invalidPadding
			}
			i -= 1
		}

		return Array(data[0..<(data.count - padLength)])
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
			var p = decryptBlock(c)
			SPN.xorBlock(&p, &prevC)
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
			result = decryptRound(&result, &subkeys[roundNum], &sboxes[roundNum], roundNum == SPN.roundsCount - 1)
		}		
		return result;
	}

	func decryptRound(_ input: inout [UInt8], _ subKey: inout [UInt8], _ sboxes: inout [SBox], _ skipPermutation: Bool) -> [UInt8] {
		var result = input
		if(!skipPermutation) {
			let pboxInput = assembleUInt64(result)
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

		SPN.xorBlock(&result, &subKey)

		return result
	}

	func xorWithLastSubKey(_ input: [UInt8]) -> [UInt8] {
		var result = input
		for i in (0..<input.count) {
			result[i] ^= subkeys[SPN.roundsCount][i]
		}
		return result
	}

	static func xorBlock(_ lhs: inout [UInt8], _ rhs: inout [UInt8]) {
		for i in (0..<lhs.count) {
			lhs[i] ^= rhs[i]
		}		
	}

	var sboxes: [[SBox]]
	let pbox: PBox
	var subkeys: [[UInt8]];
	let masterKey: [UInt8]
}

enum SPNError: Error {
	case notEnoughKeyMaterial
    case unpaddedInput
    case invalidPadding
}