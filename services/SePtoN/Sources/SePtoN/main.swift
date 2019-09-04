import Foundation

// let spn = SPN([7, 6, 5, 4, 3, 2, 1, 0])

// // let plain: [UInt8] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
// let plain : [UInt8] =  Array(repeating: 0, count: 1_000_000)
// let iv: [UInt8] = SPN.generateIV()

// // print(plain)
// let enc = try! spn.encryptWithPadding(plain, iv)
// // print(enc)
// let dec = try! spn.decryptWithPadding(enc)
// // print(dec)

// exit(0)

let arguments = CommandLine.arguments
let hostArg = arguments.dropFirst(1).first
let portArg = arguments.dropFirst(2).first

let defaultHost = "127.0.0.1"
let defaultPort = 31337

let host: String, port: Int

switch (hostArg, hostArg.flatMap(Int.init), portArg.flatMap(Int.init)) {
case (.some(let h), _ , .some(let p)):
    host = h; port = p
case (_, .some(let p), _):
    host = defaultHost; port = p
default:
    host = defaultHost; port = defaultPort
}

let septon = SePtoN(host, port)
septon.start()

RunLoop.current.run()   