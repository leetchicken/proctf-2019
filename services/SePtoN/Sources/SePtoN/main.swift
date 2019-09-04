import Foundation

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