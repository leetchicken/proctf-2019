using System;
using System.IO;
using Uploader.Models;
using Uploader.Unpackers;

namespace Uploader.Extensions
{
    public static class UnpackerExtensions
    {
        public static bool TryUnpack(this IUnpacker unpacker, out Playlist playlist, Stream playlistStream)
        {
            try
            {
                return (playlist = unpacker.Unpack(playlistStream)) == null;
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                playlist = null;
                return false;
            }
        }
    }
}