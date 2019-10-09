using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using Microsoft.AspNetCore;
using Microsoft.AspNetCore.Hosting;
using Uploader.Unpackers;

namespace Uploader
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var archive = ZipFile.OpenRead("/Users/ximik/Desktop/music_playlist.zip");
            var str = string.Join(", ", archive.Entries.Select(x => $"{x.FullName}: {x.Length}"));
            var unpacker = new Unpacker();
            Console.WriteLine(str);
            using var fs = new FileStream("/Users/ximik/Desktop/music_playlist.zip", FileMode.Open);
            unpacker.Unpack(fs);


            //CreateWebHostBuilder(args).Build().Run();
        }

        public static IWebHostBuilder CreateWebHostBuilder(string[] args) =>
            WebHost.CreateDefaultBuilder(args)
                .UseKestrel()
                .UseStartup<Startup>();
    }
}