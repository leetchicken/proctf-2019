using System;
using System.IO;

namespace Uploader
{
    public class Utils
    {
        public static string PlaylistsPath
            => Path.Combine(AppDomain.CurrentDomain.BaseDirectory, Constants.PlaylistsPath);
        
        public static void CreateDirectoryIfNotExists(string path)
        {
            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory(path);
            }
        }

        
    }
}