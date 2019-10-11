using Uploader.Models;

namespace Uploader.Storages
{
    public interface IStorage
    {
        void Store(Playlist playlist);
        Playlist Get(int id);
    }
}