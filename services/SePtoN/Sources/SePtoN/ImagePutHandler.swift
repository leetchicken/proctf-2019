import NIO
import NIOExtras
import BigInt
import Foundation

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
        case done
    }

    private var state: State = .keyExchange
    private var spn: SPN?

    // public func channelActive(context: ChannelHandlerContext) {
    //     print("Client \(context.remoteAddress!) connected")
    // }

    public func channelRead(context: ChannelHandlerContext, data: NIOAny) {

        if case .keyExchange = self.state {
            processKeyExchange(context, data)
            state = .imageTransfer
            return
        }

        if case .imageTransfer = self.state {
            processImageTransfer(context, data)
            state = .done
            // context.close(promise: nil)
            return
        }
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

        let xB = BigUInt.randomInteger(withExactWidth: 20 * 8)
        let yB = g.power(xB, modulus: p)

        let key = yA.power(xB, modulus: p)
        let keyMaterial = Array(key.serialize())

        guard let masterKey = try? SPN.CalcMasterKey(keyMaterial) else {
            print("Failed to generate masterKey from DH-derived key \(key)")
            context.close(promise: nil)
            return
        }
        spn = SPN(masterKey)

        var yBbuffer = context.channel.allocator.buffer(capacity: 128 + 1)
        yBbuffer.writeBytes(yB.serialize())

        context.writeAndFlush(self.wrapOutboundOut(yBbuffer), promise: nil)
    } 

    public func processImageTransfer(_ context: ChannelHandlerContext, _ data: NIOAny) {
        var byteBuffer = unwrapInboundIn(data)

        guard let c = byteBuffer.readBytes(length: byteBuffer.readableBytes) else {
            context.close(promise: nil)
            return
        }

        guard var imageBytes = try? spn?.decryptWithPadding(c) else {
            print("!! Failed to decryptWithPadding c of \(c.count) bytes")
            context.close(promise: nil)
            return
        }

        if(!tryInjectWatermark(&imageBytes, generateWatermark(imageBytes))) {
            print("!! Failed to injectWatermark, maybe it's not an image?")
            context.close(promise: nil)
            return
        }


        guard let newC = try? spn?.encryptWithPadding(imageBytes, SPN.generateIV()) else {        
            print("!! Failed to encryptWithPadding newImageData of \(imageBytes.count) bytes")
            context.close(promise: nil)
            return
        }

        guard let (id, fileUrl) = try? filesProvider.generateNextFilePath() else {
            print("!! Can't get next filePath")
            context.close(promise: nil)
            return
        }

        if (try? Data(newC).write(to: fileUrl, options: .atomic)) == nil {
            print("!! Failed to save newC of \(newC.count) bytes to \(fileUrl):")
            context.close(promise: nil)
            return
        }

        var result = context.channel.allocator.buffer(capacity: 4)
        result.writeInteger(id)

        context.writeAndFlush(self.wrapOutboundOut(result), promise: nil)
    }


    private func generateWatermark(_ imageBytes: [UInt8]) -> [UInt8] {
        let waterMark = imageBytes.chunks(4).map { buf in buildUInt32(buf, 0)}.reduce(0, { x, y in x ^ y} )

        let buf: [UInt8] = Array("The Gods demand justice: \(waterMark)".utf8)

        var result = [UInt8]()
        result.reserveCapacity(buf.count * 8)

        return buf.flatMap { b in
            return (0..<8).map { bitNum in 
                let offset = (8-1 - bitNum)
                return (b & (1 << offset)) >> offset
            }
        }
    }

    private func tryInjectWatermark(_ imageBytes: inout [UInt8], _ watermarkPatternBits: [UInt8]) -> Bool {
        guard imageBytes.count >= 14 else { return false }
        guard imageBytes[0] == 0x42 && imageBytes[1] == 0x4D else { return false }
        guard buildUInt32(imageBytes, 2) == imageBytes.count else { return false }

        let pixelsOffset = buildUInt32(imageBytes, 0x0A)
        if(pixelsOffset >= imageBytes.count) { return false }
        
        for i in (Int(pixelsOffset)..<imageBytes.count) {
            let j = (i - Int(pixelsOffset)) % watermarkPatternBits.count
            imageBytes[i] &= 0xFE
            imageBytes[i] |= watermarkPatternBits[j]
        }
        return true
    }

    private func buildUInt32(_ byteArray: [UInt8], _ offset: Int) -> UInt32 {
        var result : UInt32 = 0
        for i in (offset..<offset+MemoryLayout<UInt32>.size) {
            if (i >= byteArray.count) {
                break
            }
            result |= (UInt32(byteArray[i]) << (8 * (i - offset)))
        }
        return result
    }

    public func channelReadComplete(context: ChannelHandlerContext) {
        context.flush()
    }

    public func errorCaught(context: ChannelHandlerContext, error: Error) {
        print("error: ", error)
        context.close(promise: nil)
    }
}

extension Array {
    func chunks(_ chunkSize: Int) -> [[Element]] {
        return stride(from: 0, to: self.count, by: chunkSize).map {
            Array(self[$0..<Swift.min($0 + chunkSize, self.count)])
        }
    }
}