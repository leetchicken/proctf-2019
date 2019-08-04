import NIO
import NIOExtras
import BigInt
import Foundation

public final class ImagePutHandler: ChannelInboundHandler {

// Params from RFC 5114
    let p :BigUInt = "124325339146889384540494091085456630009856882741872806181731279018491820800119460022367403769795008250021191767583423221479185609066059226301250167164084041279837566626881119772675984258163062926954046545485368458404445166682380071370274810671501916789361956272226105723317679562001235501455748016154805420913"
    let g :BigUInt = "115740200527109164239523414760926155534485715860090261532154107313946218459149402375178179458041461723723231563839316251515439564315555249353831328479173170684416728715378198172203100328308536292821245983596065287318698169565702979765910089654821728828592422299160041156491980943427556153020487552135890973413"

    public typealias InboundIn = ByteBuffer
    public typealias OutboundOut = ByteBuffer

    // private enum State {
    //     case waiti
    //     case waitingForFrame(length: Int)
    // }

    // public func channelActive(context: ChannelHandlerContext) {
    //     let remoteAddress = context.remoteAddress!
    //     let channel = context.channel      
    //     context.writeAndFlush(self.wrapOutboundOut(buffer), promise: nil)
    // }

    public func channelRead(context: ChannelHandlerContext, data: NIOAny) {

// Expecting from Client no more than 128 bytes of his DH part: g^a mod p
        var byteBuffer = unwrapInboundIn(data)
        if byteBuffer.readableBytes > 128 * 8 {
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
        print("yB: ", yB)

        let key = yA.power(xB, modulus: p)
        print("Key: ", key)

        var yBbuffer = context.channel.allocator.buffer(capacity: 128)
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