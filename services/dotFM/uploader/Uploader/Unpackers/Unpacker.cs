using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using Uploader.Extensions;
using Uploader.Models;

namespace Uploader.Unpackers
{
    public class Unpacker: IUnpacker
    {
        private const string PlaylistDescriptionFile = "playlist.m3u";
        private const int DescriptionFileSizeLimit = 1024 * 16;
        private const int EntryMaxLimit = 1024 * 1024;
        
        public Playlist Unpack(Stream compressedFile)
        {
            var archive = new ZipArchive(compressedFile);
            ZipArchiveEntry archiveDescriptionFile;
            try
            {
                 archiveDescriptionFile = archive.Entries.First(x => x.FullName == PlaylistDescriptionFile);
            }
            catch (InvalidOperationException e)
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
                fs.Read(binaryText);
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

            foreach (var entry in archive.Entries)
            {
                if (playlist.Tracks.ContainsValue(entry.FullName))
                {
                    if (entry.Length < EntryMaxLimit)
                    {
                        var trackBytes = new byte[entry.Length];
                        using var fs = entry.Open();
                        fs.Read(trackBytes);
                        var newTrackName = string.Concat(
                            new SHA1Managed()
                                .ComputeHash(trackBytes)
                                .Select(x => x.ToString("x2"))
                            ) + ".mp3";
                        playlist.AudioFiles[newTrackName] = new AudioFile(trackBytes);
                        foreach (var (key, value) in playlist.Tracks.ToList())
                            if (value == entry.FullName)
                                playlist.Tracks[key] = newTrackName;
                    }
                }
            }

            return playlist;
        }
    }
}