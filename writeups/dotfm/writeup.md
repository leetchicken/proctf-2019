## .FM

This service is an emulation of infinite field with radio stations.
You can create a station by uploading an archive with music and playlist file.
Uploading archive leads to this actions:

1) Unpacking zip archive to the mp3.files.
2) Extracting an ID3 tag from the archive.
3) Saving image from ID tag.
4) Creating ability to lookup for tracks in playlist.
5) Returning playlist GUID.

This service is written using "microservices architecture" paradigm.

### Tech specs

Service consists of two parts.

##### Frontend (front_proxy)
Written in Python 3.7. 
Available methods:  

`GET /channel?id=<id>&num=<n>` -> Fetches music from filesystem by asking file mapping from backend service.    
<id> can be only a GUID (uuid v4)  
  
`GET /image/<img>.png` -> Get album picture.  
  
`POST /channel` -> Upload music.  
Request body should be a *.zip archive.  
\* optional: Position header for visualization:  
`Position: x,y`. 

Also, service has another methods, but they are used just for the visualization.
`GET /lookup` -> list of all stations ids in radius = 150 points
And some served static :)


##### Backend (uploader)
Written in C# (.net Core 3.0).
Available methods:

`GET /playlist?playlist_id=<id>` -> Get audiofiles paths in filesystem
`POST /playlist` -> Create a playlist by sent archive


##### Services interaction

Playlist archive structure: 
```
----|
    |playlist.m3u  
    |track1.mp3  
    |trackN.mp3  
```

*.m3u file structure:

```
#EXTM3U

#EXTINF:42, Some Group name - Some track name
track1.mp3

#EXTINF:42, Some Group name 2 - Some track name 2
track2.mp3

```

Let's look a bit more attentively:
Main workflows are:

`POST /channel (frontend) -> POST /playlist (backend) -> store file in FS`
`GET /channel (frontend) -> GET /playlist (backend) -> fetch files from frontend`

### Vuln

##### Path traversal
While music parsing and creating image we combine mp3 tags: Artist, Album and track. There is no filtering.
So, you could bypass it into `Path.Combine` and create png file in other path.

Surprisingly, if you store correct text in image file tags, you can parse it as a playlist.

##### Frontend file parsing
Main logic is in searching <sha1>.mp3.
Playlist always has full hases, but search is done by prefixes.
So, you could list all files by bypsassing short sha1 prefixes of files.
