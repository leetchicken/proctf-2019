using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using Uploader.Models;

namespace Uploader.Unpackers
{
    public class Unpacker: IUnpacker
    {
        private const string PlaylistDescriptionFile = "playlist.m3u";
        private const int DescriptionFileSizeLimit = 1024 * 16;
        
        public Playlist Unpack(Stream compressedFile)
        {
            var archive = new ZipArchive(compressedFile);
            var archiveDescriptionFile = archive.Entries.First(x => x.Name == PlaylistDescriptionFile);

            if (archiveDescriptionFile == null)
            {
                return null;
            }

            if (archiveDescriptionFile.Length > DescriptionFileSizeLimit)
            {
                return null;
            }

            var binaryText = new Span<byte>(new byte[archiveDescriptionFile.Length]);
            using (var fs = archiveDescriptionFile.Open())
            {
                var res = fs.Read(binaryText);
                Console.WriteLine(res);
            }

           
            var m3uText = Encoding.UTF8.GetString(binaryText);
            Playlist playlist;
            
            try
            {
                playlist = Playlist.FromM3U(m3uText);
            }
            catch (ArgumentException e)
            {
                Console.WriteLine(e);
                return null;
            }
            
            Console.WriteLine(playlist);

            foreach (var entry in archive.Entries)
            {
                
            }
            
            return new Playlist(0);
        }
    }
}