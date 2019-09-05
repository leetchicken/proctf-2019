import NIO
import NIOExtras
import Foundation

public class SePtoN {

	let host: String

	let portPut: Int
	let portGet: Int

	init(_ host: String, _ basePort: Int){
		self.host = host
		self.portPut = basePort
		self.portGet = basePort + 1

		self.groupPut = MultiThreadedEventLoopGroup(numberOfThreads: 2 * System.coreCount)
		self.groupGet = MultiThreadedEventLoopGroup(numberOfThreads: 2 * System.coreCount)
	}

	let filesProvider = FilesProvider()

	var groupPut: EventLoopGroup
	var groupGet: EventLoopGroup

	deinit {
		try? groupPut.syncShutdownGracefully()
		try? groupGet.syncShutdownGracefully()
	}
	

	public func start(){		
		let _ = startServer(self.groupPut, { ImagePutHandler(self.filesProvider) }, host, portPut)
		let _ = startServer(self.groupGet, { ImageGetHandler(self.filesProvider) }, host, portGet)		
	}

	private func startServer<THandler: ChannelInboundHandler>(_ group: EventLoopGroup, _ handlerFactory: @escaping () -> THandler, _ host: String, _ port: Int) -> Channel
	{
		let bootstrap = ServerBootstrap(group: group)
		    .serverChannelOption(ChannelOptions.backlog, value: 1024)
		    .serverChannelOption(ChannelOptions.socket(SocketOptionLevel(SOL_SOCKET), SO_REUSEADDR), value: 1)

		    .childChannelInitializer { channel in
		    	// channel.pipeline.addHandler(DebugOutboundEventsHandler()).flatMap { v in
		    		// channel.pipeline.addHandler(DebugInboundEventsHandler()).flatMap { v in
				        channel.pipeline.addHandler(ByteToMessageHandler(LengthFieldBasedFrameDecoder(lengthFieldLength: .two))).flatMap { v in
				            channel.pipeline.addHandler(LengthFieldPrepender(lengthFieldLength: .two)).flatMap { v in
				                channel.pipeline.addHandler(handlerFactory())
				            }
				        }
				    // }
			    // }
		    }

		    .childChannelOption(ChannelOptions.socket(IPPROTO_TCP, TCP_NODELAY), value: 1)
		    .childChannelOption(ChannelOptions.socket(SocketOptionLevel(SOL_SOCKET), SO_REUSEADDR), value: 1)
		    .childChannelOption(ChannelOptions.maxMessagesPerRead, value: 16)
		    .childChannelOption(ChannelOptions.recvAllocator, value: AdaptiveRecvByteBufferAllocator())    
		// defer {
		//     try! group.syncShutdownGracefully()
		// }

		let channel: Channel
		do {
		    try channel = bootstrap.bind(host: host, port: port).wait()
		}
		catch {
		    fatalError("failed to start server: \(error)")
		}
		print("Server started and listening on \(channel.localAddress!)")

		return channel
	}
}