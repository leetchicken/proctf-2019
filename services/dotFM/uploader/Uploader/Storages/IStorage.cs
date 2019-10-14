using System;
using System.Collections.Generic;
using Uploader.Models;

namespace Uploader.Storages
{
    public interface IStorage
    {
        Guid Store(Playlist playlist);
        List<string> Get(Guid guid);
    }
}