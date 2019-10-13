using System.IO;

namespace Uploader
{
    public class Constants
    {
        public static readonly string RootPath = "storage";
        public static readonly string MusicPath = Path.Combine(RootPath, "music");
        public static readonly string ImagesPaths = Path.Combine(RootPath, "images");
        public static readonly string PlaylistsPath = Path.Combine(RootPath, "playlists");
    }
}