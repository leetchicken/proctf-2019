using System;
using System.Collections.Generic;
using System.Linq;

namespace Uploader.Models
{
    public class Playlist
    {
        private readonly Dictionary<string, string> tracks;
        private const string M3UHeader = "#EXTM3U";
        private const string M3UINFOHeader = "#EXTINF";

        public Playlist(int size)
        {
            tracks = new Dictionary<string, string>(size);
        }

        private void AddTrack(string trackName, string trackPath) => 
            tracks[trackName] = trackPath;

        public override string ToString() => 
            string.Join(", ", tracks.Select(x => $"{x.Key}: {x.Value}"));


        public static Playlist FromM3U(string m3uContent)
        {
            var m3uLines = m3uContent.Split('\n', StringSplitOptions.RemoveEmptyEntries);
            if (m3uLines.Length < 1)
            {
                throw new ArgumentException("Empty m3u file");
            }

            if (m3uLines[0] != M3UHeader)
            {
                throw new ArgumentException("Bad m3u file formatting");
            }
            
            var tracks = m3uLines
                .Select((line, index) => (index, line))
                .Skip(1)
                .Where(x => x.index % 2 == 1)
                .Select(x => (x.line, m3uLines[x.index + 1]))
                .ToList();

            var playlist = new Playlist(tracks.Count);
            foreach (var (description, link) in tracks)
            {
                if (!description.StartsWith(M3UINFOHeader))
                {
                    throw new ArgumentException("Bad track description!");
                }

                var trackName = description.Split(",", StringSplitOptions.RemoveEmptyEntries);
                if (trackName.Length < 2)
                {
                    throw new ArgumentException("Could not find track info");
                }

                playlist.AddTrack(trackName[1].Trim(' '), link);
            }

            return playlist;
        }
    }
}