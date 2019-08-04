import NIO
import NIOExtras
import Foundation

public final class ImageGetHandler: ChannelInboundHandler {
    public typealias InboundIn = ByteBuffer
    public typealias OutboundOut = ByteBuffer

    let dirSize: UInt32 = 500
    let storageDir = "./storage"

    public func channelRead(context: ChannelHandlerContext, data: NIOAny) {
        var byteBuffer = unwrapInboundIn(data)
        if byteBuffer.readableBytes > (dirSize.bitWidth / 8) {
            context.close(promise: nil)
            return
        }
        guard let fileNumBytes = byteBuffer.readBytes(length: byteBuffer.readableBytes) else {
            context.close(promise: nil)
            return
        }
        var fileId : UInt32 = 0
        for byte in fileNumBytes {
            fileId = fileId << 8
            fileId = fileId | UInt32(byte)
        }

        let fileDir = fileId / dirSize
        let fileNum = fileId % dirSize

        let filePath = "\(storageDir)/\(fileDir)/\(fileNum)"

        let fm  = FileManager.default
        
        
        print("reading file \(filePath)")
        guard let data = try fm.contents(atPath: filePath) else {
            print("Failed to read file: \(filePath)")
            context.close(promise: nil)                
            return
        }
        print("read \(data.count)")        
    }

    public func channelReadComplete(context: ChannelHandlerContext) {
        context.flush()
    }

    public func errorCaught(context: ChannelHandlerContext, error: Error) {
        print("error: ", error)
        context.close(promise: nil)
    }
}