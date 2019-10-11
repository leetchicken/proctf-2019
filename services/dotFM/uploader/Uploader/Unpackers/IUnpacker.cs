using System.IO;
using Uploader.Models;

namespace Uploader.Unpackers
{
    public interface IUnpacker
    {
        Playlist Unpack(Stream compressedFile);
    }
}