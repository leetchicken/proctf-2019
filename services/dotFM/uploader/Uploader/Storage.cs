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
        }

        private void CreateImages(Playlist playlist)
        {
            foreach (var audioFile in playlist.AudioFiles)
            {
                var imagePath = Path.Combine(
                    Constants.ImagesPaths, 
                    audioFile.Value.GetTrackIdentity() + ".png");
                using var fs = new FileStream(imagePath, FileMode.Create);
                fs.Write(audioFile.Value.GetImage());
            }
        }

        private void CreateTracks(Playlist playlist)
        {
            foreach (var audioFile in playlist.AudioFiles)
            {
                var audioPath = Path.Combine(
                    Constants.MusicPath, audioFile.Key + ".mp3"
                );
            }
        }
    }
}