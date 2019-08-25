import NIO
import NIOExtras
import BigInt
import Foundation
import SwiftGD

public final class ImagePutHandler: ChannelInboundHandler {

// Params from RFC 5114
    let p :BigUInt = "124325339146889384540494091085456630009856882741872806181731279018491820800119460022367403769795008250021191767583423221479185609066059226301250167164084041279837566626881119772675984258163062926954046545485368458404445166682380071370274810671501916789361956272226105723317679562001235501455748016154805420913"
    let g :BigUInt = "115740200527109164239523414760926155534485715860090261532154107313946218459149402375178179458041461723723231563839316251515439564315555249353831328479173170684416728715378198172203100328308536292821245983596065287318698169565702979765910089654821728828592422299160041156491980943427556153020487552135890973413"

    let filesProvider: FilesProvider

    init(_ filesProvider: FilesProvider){
        self.filesProvider = filesProvider
    }

    public typealias InboundIn = ByteBuffer
    public typealias OutboundOut = ByteBuffer

    private enum State {
        case keyExchange
        case imageTransfer
    }

    private var state: State = .keyExchange
    private var spn: SPN?

    // public func channelActive(context: ChannelHandlerContext) {
    //     let remoteAddress = context.remoteAddress!
    // }

    public func channelRead(context: ChannelHandlerContext, data: NIOAny) {

        if case .keyExchange = self.state {
            processKeyExchange(context, data)
            state = .imageTransfer
            return
        }

        if case .imageTransfer = self.state {
            processImageTransfer(context, data)

            context.close(promise: nil)
            return
        }
    }

    public func processImageTransfer(_ context: ChannelHandlerContext, _ data: NIOAny) {
        print()
        print("IMAGE TRANSFER")

        var byteBuffer = unwrapInboundIn(data)

        guard let c = byteBuffer.readBytes(length: byteBuffer.readableBytes) else {
            context.close(promise: nil)
            return
        }

        // let start = DispatchTime.now()

        guard let imageBytes = try? spn?.decryptWithPadding(c) else {
            print("!! Failed to decryptWithPadding c of \(c.count) bytes")
            context.close(promise: nil)
            return
        }

        // let end = DispatchTime.now()
        // let nanoTime = end.uptimeNanoseconds - start.uptimeNanoseconds
        // let timeInterval = Double(nanoTime) / 1_000_000_000

        // print("Decrypted \(imageBytes.count) bytes in \(timeInterval) sec")

        guard let image = try? Image(data: Data(imageBytes), as: .bmp) else {
            print("!! Failed to parse decrypted image bytes as valid Image")
            context.close(promise: nil)
            return
        }

        print("GOT VALID IMAGE: \(image.size)")

        guard let newImageData = try? image.export(as: .bmp(compression: false)) else {
            print("!! Failed to serialze image of size \(image.size) ")
            context.close(promise: nil)
            return
        }

        guard let newC = try? spn?.encryptWithPadding(Array(newImageData), SPN.generateIV()) else {
            print("!! Failed to encryptWithPadding newImageData of \(newImageData.count) bytes")
            context.close(promise: nil)
            return
        }

        guard let destination = try? filesProvider.generateNextFilePath() else {
            print("!! Can't get next filePath")
            context.close(promise: nil)
            return
        }

        // guard let dummy = try? Data(newC).write(to: destination, options: .atomic) else {
        //     print("!! Failed to save newC of \(newC.count) bytes to \(destination):")
        //     context.close(promise: nil)
        //     return
        // }

        guard let dummy = try? Data(newC).write(to: destination, options: .atomic) else {
            print("!! Failed to save newC of \(newC.count) bytes to \(destination):")
            context.close(promise: nil)
            return
        }

        try? Data(imageBytes).write(to: URL(fileURLWithPath: "source.bmp"))
        try? newImageData.write(to: URL(fileURLWithPath: "processed.bmp"))

        print("SAVED DATA: \(destination.path), size: \(newC.count)")

        var result = context.channel.allocator.buffer(capacity: 4)
        result.writeBytes([0, 0, 0, 0]) //TODO watermark image, encrypt it back, save to file and respond with fileId

        context.write(self.wrapOutboundOut(result), promise: nil)
    }

    public func processKeyExchange(_ context: ChannelHandlerContext, _ data: NIOAny) {

// Expecting from Client no more than 128 bytes of his DH part: g^a mod p
        var byteBuffer = unwrapInboundIn(data)
        if byteBuffer.readableBytes > 128 + 1 {
            context.close(promise: nil)
            return
        }
        guard let newData = byteBuffer.readBytes(length: byteBuffer.readableBytes) else {
            context.close(promise: nil)
            return
        }
        
        let yA = BigUInt(Data(newData))
        // print(newData)
        // print("yA: ", yA)
        // print()

        let xB = BigUInt.randomInteger(withExactWidth: 20 * 8)
        let yB = g.power(xB, modulus: p)
        // print("yB: ", yB)

        let key = yA.power(xB, modulus: p)
        let keyMaterial = Array(key.serialize())
        // print(keyMaterial)
        print("Key: ", key)
        print()

        guard let masterKey = try? SPN.CalcMasterKey(keyMaterial) else {
            print("Failed to generate masterKey from DH-derived key \(key)")
            context.close(promise: nil)
            return
        }
        spn = SPN(masterKey)

        var yBbuffer = context.channel.allocator.buffer(capacity: 128 + 1)
        yBbuffer.writeBytes(yB.serialize())

// Responding with our g^b mod p back to Client
        context.write(self.wrapOutboundOut(yBbuffer), promise: nil)
    } 

    public func channelReadComplete(context: ChannelHandlerContext) {
        context.flush()
    }

    public func errorCaught(context: ChannelHandlerContext, error: Error) {
        print("error: ", error)
        context.close(promise: nil)
    }
}