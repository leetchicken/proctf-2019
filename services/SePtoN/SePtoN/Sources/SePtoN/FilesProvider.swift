import Foundation

public final class FilesProvider {

	let dispatchQueue: DispatchQueue
	let dataDirectory: URL	

	let dataDirName = "data"
	let filesGroupSize = 1000

	var nextFileId: Int = 0

	init() {
		dispatchQueue = DispatchQueue(label: "imagesPutLockQueue")		

		let fileManager = FileManager.default
		dataDirectory = URL(fileURLWithPath: fileManager.currentDirectoryPath).appendingPathComponent(dataDirName)
		
		do {
			try fileManager.createDirectory(at: dataDirectory, withIntermediateDirectories: true)
			let lastIdsGroup = try fileManager
						.contentsOfDirectory(at: dataDirectory, includingPropertiesForKeys: nil, options: [])
						.compactMap { Int($0.lastPathComponent) }
						.max()

			if (lastIdsGroup == nil) {				
				nextFileId = 0
				return
			}

			let lastIdsGroupPath = dataDirectory.appendingPathComponent(String(lastIdsGroup!))			

			let lastFileId = try fileManager
						.contentsOfDirectory(at: lastIdsGroupPath, includingPropertiesForKeys: nil, options: [])
						.compactMap { Int($0.lastPathComponent) }
						.max()

			if(lastFileId != nil) {				
				nextFileId = lastFileId! + 1
			}
		} catch let error as NSError {
            print("Failed to init FilesProvider: \(error.localizedDescription)")            
        }
	}

	public func generateNextFilePath() throws -> URL {
		var path : URL!
		self.dispatchQueue.sync {
			path = getFilePathForId(nextFileId)
			nextFileId += 1			
		}

		try FileManager.default.createDirectory(at: path.deletingLastPathComponent(), withIntermediateDirectories: true)
		return path
	}

	public func getFilePathForId(_ id: Int) -> URL {
		let fileIdsGroup = (id / filesGroupSize) * filesGroupSize;
		return dataDirectory.appendingPathComponent(String(fileIdsGroup)).appendingPathComponent(String(nextFileId))		
	}
}