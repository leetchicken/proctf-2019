using System;
using System.IO;
using System.Linq;
using IdSharp.Tagging.ID3v2;

namespace Uploader.Models
{
    public class AudioFile
    {
        private readonly byte[] content;
        private readonly ID3v2Tag tag;

        public string Identity => tag.Artist + tag.Album;

        public AudioFile(byte[] content)
        {
            this.content = content;
            this.tag = new ID3v2Tag(new MemoryStream(content));
        }

        public byte[] GetImage()
        {
            var tag = new ID3v2Tag(new MemoryStream(content));

            if (tag.PictureList.Count == 0)
                return null;

            var picture = tag.PictureList.First();
            return picture.PictureData;
        }

        public byte[] GetContent() => content;
    }
}