using System;
using System.Collections.Concurrent;
using System.IO;
using Uploader.Extensions;
using Uploader.Models;


namespace Uploader.Storages
{
    public class Storage: IStorage
    {
        private ConcurrentDictionary<string, string> MusicCache;
        private readonly string workingPath;

        public Storage(string workingPath)
        {
            this.workingPath = workingPath;
        }

        private void CreateImages(Playlist playlist)
        {
            foreach (var (_, value) in playlist.AudioFiles)
            {
                using var fs = CreateFileStream(Path.Combine(Constants.ImagesPaths, value.GetFileIdentity() + ".png"));
                fs.Write(value.GetImage());
            }
        }

        private void CreateTracks(Playlist playlist)
        {
            foreach (var (key, value) in playlist.AudioFiles)
            {
                using var fs = CreateFileStream(Path.Combine(Constants.MusicPath, key));
                fs.Write(value.GetContent());
            }
        }
        
        private Stream CreateFileStream(string path) => 
            new FileStream(Path.Join(workingPath, path), FileMode.CreateNew);

        private Playlist CreateUniquePlaylist()
        {
            return null;
        }

        public void Store(Playlist playlist)
        {
            CreateDirsIfNotExists();
            
            var playlistId = Guid.NewGuid();
            
            CreateImages(playlist);
            CreateTracks(playlist);
            //todo create playlist file 
        }

        private static void CreateDirsIfNotExists()
        {
            Utils.CreateDirectoryIfNotExists(Constants.PlaylistsPath);
            Utils.CreateDirectoryIfNotExists(Constants.MusicPath);
            Utils.CreateDirectoryIfNotExists(Constants.ImagesPaths);
        }

        public Playlist Get(int id)
        {
            throw new System.NotImplementedException();
        }
    }
}