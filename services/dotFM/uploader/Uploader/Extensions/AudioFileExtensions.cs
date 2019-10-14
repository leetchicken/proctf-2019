using System;
using System.IO;
using IdSharp.Tagging.ID3v2;
using Uploader.Models;

namespace Uploader.Extensions
{
    public static class AudioFileExtensions
    {
        public static string GetFileIdentity(this AudioFile file)
        {
            var tag = new ID3v2Tag(new MemoryStream(file.GetContent()));
            var identity = tag.Artist + tag.Album;
            return identity == "" ? $"{Guid.NewGuid()}" : identity;
        }
    }
}