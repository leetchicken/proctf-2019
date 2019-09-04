public final class SBox {
	static let bitSize = 4

	var mapping = [UInt8](repeating: 0, count: 1 << bitSize)
	var invmapping = [UInt8](repeating: 0, count: 1 << bitSize)

	convenience init(_ output: [UInt8]) {
		self.init((0..<output.count).map {UInt8($0)}, output)
	}

	init(_ input: [UInt8], _ output: [UInt8]) {
		for (i, element) in input.enumerated() {
			let outputElement = output[i]
			mapping[element] = outputElement
			invmapping[outputElement] = element
		}
	}

	func substitute(_ input: UInt8) -> UInt8{
		return mapping[input]
	}

	func invSubstitute(_ input: UInt8) -> UInt8 {
		return invmapping[input]
	}
}


extension Array {
    subscript (index: UInt8) -> Element {
        get {
            let intIndex : Int = Int(index)
            return self[intIndex]
        }
        set {
        	let intIndex : Int = Int(index)
            self[intIndex] = newValue
        }
    }
}