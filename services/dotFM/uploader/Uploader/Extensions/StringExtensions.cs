using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Uploader.Extensions
{
    public static class StringExtensions
    {
        private static readonly HashSet<char> badSymbols = new HashSet<char>(
            Path.GetInvalidFileNameChars());
        
        private static char BadSymbolReplacement = '_';
        
        public static string CleanupFromBadPathSymbols(this string str) => 
            string.Concat(str.Select(x => badSymbols.Contains(x) ? BadSymbolReplacement : x));
    }
}