using System;
using Uploader.Models;

namespace Uploader.Storages
{
    public interface IStorage
    {
        Guid Store(Playlist playlist);
        Playlist Get(int id);
    }
}