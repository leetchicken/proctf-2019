using System;
using System.IO;
using System.Linq;
using IdSharp.Tagging.ID3v2;

namespace Uploader.Models
{
    public class AudioFile: IEquatable<AudioFile>
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


        public bool Equals(AudioFile other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return Equals(content, other.content);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != GetType()) return false;
            return Equals((AudioFile) obj);
        }

        public override int GetHashCode()
            => (content != null ? content.GetHashCode() : 0);
    }
}