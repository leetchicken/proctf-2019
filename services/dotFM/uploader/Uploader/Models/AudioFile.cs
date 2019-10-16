using System.IO;
using System.Linq;
using IdSharp.Tagging.ID3v2;
using Uploader.Extensions;

namespace Uploader.Models
{
    public class AudioFile
    {
        private const string UnknownIdentity = "unknown";
        private const string PngMimeType = "image/png";

        public AudioFile(byte[] content)
        {
            this.content = content;
            var tag = new ID3v2Tag(new MemoryStream(content));

            if (tag.PictureList.Count != 0 && tag.PictureList.First().MimeType == PngMimeType)
                image = tag.PictureList.First().PictureData;

            identity = tag.TagFields().Any(x => x == "") 
                ? UnknownIdentity 
                : string.Join("_", tag.TagFields());
        }

        public byte[] GetImage() => image;

        public byte[] GetContent() => content;

        public override string ToString() => identity;
        
        private readonly byte[] content;
        private readonly byte[] image;
        private readonly string identity;
    }
}