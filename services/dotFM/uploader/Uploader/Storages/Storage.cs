using System;
using System.Collections.Concurrent;
using System.IO;
using Uploader.Extensions;
using Uploader.Models;


namespace Uploader.Storages
{
    public class Storage: IStorage
    {
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

        private Guid CreatePlaylistFile(Playlist playlist)
        {
            var playlistId = Guid.NewGuid();
            using var streamWriter = File.CreateText(Path.Combine(Constants.PlaylistsPath, playlistId + ".m3u"));
            streamWriter.Write(playlist.ToString().AsSpan());
            return playlistId;
        }
        
        private Stream CreateFileStream(string path) => 
            new FileStream(Path.Join(workingPath, path), FileMode.Create);

        public Guid Store(Playlist playlist)
        {
            CreateDirsIfNotExists();
            
            CreateImages(playlist);
            CreateTracks(playlist);
            return CreatePlaylistFile(playlist);
        }

        private static void CreateDirsIfNotExists()
        {
            Utils.CreateDirectoryIfNotExists(Constants.RootPath);
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