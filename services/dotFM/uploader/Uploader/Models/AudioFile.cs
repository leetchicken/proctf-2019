using System;
using System.IO;
using System.Linq;
using IdSharp.Tagging.ID3v2;

namespace Uploader.Models
{
    public class AudioFile
    {
        private readonly byte[] content;

        public AudioFile(byte[] content)
        {
            this.content = content;
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

        public string GetTrackIdentity()
        {
            var tag = new ID3v2Tag(new MemoryStream(content));
            var identity = tag.Artist + tag.Album;
            return identity == "" ? Guid.NewGuid().ToString() : identity;
        }
    }
}