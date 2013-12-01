
#pragma once

#include <wtf/ThreadSafeRefCounted.h>

class MediaStream : public ThreadSafeRefCounted<MediaStream>
{
public:
    class Tracks {
    public:
        int length() const { return 2; }
    };
    
    MediaStream() {
    }
    
    bool isLocal() const { return true; }
    Tracks* audioTracks() { return &_tracks; }
    
private:
    Tracks _tracks;
};

class MediaStreamSource : public ThreadSafeRefCounted<MediaStreamSource>
{
public:
};
