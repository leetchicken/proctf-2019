using System;
using System.Collections.Generic;
using System.Linq;

namespace Uploader.Models
{
    public class Playlist: IEquatable<Playlist>
    {
        public Dictionary<string, string> TrackPaths { get; }
        public Dictionary<string, AudioFile> TrackFiles { get; }
        private const string M3UINFOHeader = "#EXTINF";
        private const string M3UIdentity = "#EXTM3U";

        private Playlist(int size)
        {
            TrackPaths = new Dictionary<string, string>(size);
            TrackFiles = new Dictionary<string, AudioFile>(size);
        }

        public override string ToString() => 
            $"{M3UIdentity}\n\n{string.Join("\n\n", TrackPaths.Select(x => $"{M3UINFOHeader}:42, {x.Key}\n{x.Value}"))}";
        
        public static Playlist FromM3U(string m3uContent)
        {
            var m3uLines = m3uContent.Split('\n', StringSplitOptions.RemoveEmptyEntries);
            if (m3uLines.Length < 1)
            {
                throw new ArgumentException("Empty m3u file");
            }

            var tracks = m3uLines
                .Select((line, index) => (index, line))
                .Where(x => x.line.StartsWith(M3UINFOHeader))
                .Select(x => (x.line, m3uLines[x.index + 1]))
                .ToList();

            var playlist = new Playlist(tracks.Count);
            foreach (var (description, link) in tracks)
            {
                var trackName = description.Split(",", StringSplitOptions.RemoveEmptyEntries);
                if (trackName.Length < 2)
                {
                    throw new ArgumentException("Could not find track info");
                }
                
                playlist.TrackPaths[trackName[1].Trim(' ')] = link;
            }

            return playlist;
        }

        public bool Equals(Playlist other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return Equals(TrackPaths, other.TrackPaths) && Equals(TrackFiles, other.TrackFiles);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((Playlist) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((TrackPaths != null ? TrackPaths.GetHashCode() : 0) * 397) ^ (TrackFiles != null ? TrackFiles.GetHashCode() : 0);
            }
        }
    }
}