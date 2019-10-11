using System.Collections.Concurrent;
using System.IO;
using Uploader.Models;

namespace Uploader
{
    public class Storage
    {
        private ConcurrentDictionary<string, string> MusicCache;
        
        public Storage()
        {
            Utils.CreateDirectoryIfNotExists(Constants.PlaylistsPath);
            Utils.CreateDirectoryIfNotExists(Constants.MusicPath);
            Utils.CreateDirectoryIfNotExists(Constants.ImagesPaths);
        }

        public void AddPlaylist(Playlist playlist)
        {
            CreateImages(playlist);
            CreateTracks(playlist);
        }

        private void CreateImages(Playlist playlist)
        {
            foreach (var (_, value) in playlist.AudioFiles)
            {
                var imagePath = Path.Combine(
                    Constants.ImagesPaths, 
                    value.GetTrackIdentity() + ".png");
                using var fs = new FileStream(imagePath, FileMode.Create);
                fs.Write(value.GetImage());
            }
        }

        private void CreateTracks(Playlist playlist)
        {
            foreach (var (key, value) in playlist.AudioFiles)
            {
                var audioPath = Path.Combine(
                    Constants.MusicPath, 
                    key + ".mp3"
                );
                using var fs = new FileStream(audioPath, FileMode.Create);
                fs.Write(value.GetContent());
            }
        }
    }
}