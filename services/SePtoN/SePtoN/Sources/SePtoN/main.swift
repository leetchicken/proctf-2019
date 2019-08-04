import NIO
import NIOExtras
import BigInt
import Foundation

let spn = SPN([7, 6, 5, 4, 3, 2, 1, 0])

let plain: [UInt8] = [0, 1, 2, 3, 4, 5, 6, 7]
print(plain)
let enc = spn.encrypt(plain)
print(enc)
let dec = spn.decrypt(enc)
print(dec)


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