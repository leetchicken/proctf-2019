using System;
using Uploader.Models;

namespace Uploader.Storages
{
    public interface IStorage
    {
        Guid Store(Playlist playlist);
        Playlist Get(Guid guid);
        bool Link(Guid source, Guid target);
    }
}