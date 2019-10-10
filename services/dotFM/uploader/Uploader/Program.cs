using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using Microsoft.AspNetCore;
using Microsoft.AspNetCore.Hosting;
using Uploader.Unpackers;

namespace Uploader
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var unpacker = new Unpacker();
            using var fs = new FileStream("/Users/ximik/Desktop/sploit.zip", FileMode.Open);
            var ps = unpacker.Unpack(fs);
            var storage = new Storage();

            Console.WriteLine(ps.AudioFiles.Keys.First());

            var pic = ps.AudioFiles.First().Value.GetImage();
            SHA1 sha = new SHA1CryptoServiceProvider(); 
            Console.WriteLine(string.Concat(sha.ComputeHash(pic).Select(b => b.ToString("x2"))));
            
            storage.AddPlaylist(ps);
            using var sp = new FileStream("/Users/ximik/Desktop/vuln_image.png", FileMode.Open);
            var img = new Span<byte>(new byte[sp.Length]);
            sp.Read(img);
            var str = Encoding.UTF8.GetString(img);
            
            Console.WriteLine(string.Join(", ", str.Split('\n', StringSplitOptions.RemoveEmptyEntries).Take(5)));
            //Console.WriteLine(tag.Length);
            //CreateWebHostBuilder(args).Build().Run();
        }

        public static IWebHostBuilder CreateWebHostBuilder(string[] args) =>
            WebHost.CreateDefaultBuilder(args)
                .UseKestrel()
                .UseStartup<Startup>();
    }
}