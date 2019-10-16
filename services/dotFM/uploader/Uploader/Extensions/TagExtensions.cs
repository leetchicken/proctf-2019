using System.Collections.Generic;
using IdSharp.Tagging.ID3v2;

namespace Uploader.Extensions
{
    public static class TagExtensions
    {
        public static List<string> TagFields(this ID3v2Tag tag) => new List<string> { tag.Artist, tag.Title, tag.Album};
    }
}