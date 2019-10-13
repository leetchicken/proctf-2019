using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Uploader.Extensions;
using Uploader.Models;


namespace Uploader.Storages
{
    public class Storage : IStorage
    {
        private readonly string workingPath;
        private readonly Func<TimeSpan> expirationTime;

        public Storage(string workingPath, Func<TimeSpan> expirationTime)
        {
            this.workingPath = workingPath;
            this.expirationTime = expirationTime;
            
            var cleanupTask = new TaskFactory().StartNew(async () =>
            {
                while (true)
                {
                    await Task.Delay(TimeSpan.FromSeconds(30));
                    Cleanup();
                }
            });
        }

        private void CreateImages(Playlist playlist)
        {
            foreach (var (_, value) in playlist.TrackFiles)
            {
                using var fs = CreateFileStream(Path.Combine(Constants.ImagesPaths, value.GetFileIdentity() + ".png"));
                fs.Write(value.GetImage());
            }
        }

        private void CreateTracks(Playlist playlist)
        {
            foreach (var (key, value) in playlist.TrackFiles)
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

        public Playlist Get(Guid guid)
        {
            string file;
            try
            {
                file = Directory
                    .GetFiles(Constants.PlaylistsPath)
                    .Select(x => x.Split("/")[^1])
                    .First(x => x.Split(".")[0].ToLower() == guid.ToString().ToLower());
            }
            catch (InvalidOperationException e)
            {
                return null;
            }
            
            using var fs = new FileStream(Path.Combine(Constants.PlaylistsPath, file), FileMode.Open);
            var fileBytes = new byte[fs.Length];
            fs.Read(new Span<byte>(fileBytes));
            return Playlist.FromM3U(Encoding.UTF8.GetString(fileBytes));
        }

        public bool Link(Guid source, Guid target)
        {
            throw new NotImplementedException();
        }

        public void Cleanup()
        {
            var filesForCleanup = Directory
                .GetFiles(Constants.RootPath, "*.mp3", SearchOption.AllDirectories)
                .Where(x => File.GetCreationTime(x) + expirationTime() < DateTime.Now).ToList();

            foreach (var file in filesForCleanup) File.Delete(file);
        }
    }
}