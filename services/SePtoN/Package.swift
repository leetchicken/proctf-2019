// swift-tools-version:5.0
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "SePtoN",
    dependencies: [
          .package(url: "https://github.com/apple/swift-nio.git", from: "2.0.0"),
		  .package(url: "https://github.com/apple/swift-nio-extras.git", from: "1.0.0"),
          .package(url: "https://github.com/attaswift/BigInt.git", from: "4.0.0")
    ],
    targets: [
        .target(
            name: "SePtoN",
            dependencies: ["NIO", "NIOExtras", "BigInt"]),
    ]
)
