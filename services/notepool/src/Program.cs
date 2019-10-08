using System;
using System.IO;
using Microsoft.AspNetCore.Hosting;

namespace notepool
{
	public class Program
	{
		public static void Main(string[] args)
		{
			Directory.CreateDirectory("data");

			new WebHostBuilder().UseKestrel(options =>
			{
				options.ListenAnyIP(5073);
				options.AddServerHeader = false;
				options.Limits.KeepAliveTimeout = TimeSpan.FromSeconds(30.0);
				options.Limits.MaxRequestBodySize = 0L;
				options.Limits.MaxRequestLineSize = 4096;
				options.Limits.MaxRequestHeaderCount = 32;
				options.Limits.MaxRequestHeadersTotalSize = 8192;
				options.Limits.RequestHeadersTimeout = TimeSpan.FromSeconds(3.0);
			}).UseContentRoot(Directory.GetCurrentDirectory()).UseStartup<Startup>().Build().Run();

			LuceneIndex.Close();
		}
	}
}
