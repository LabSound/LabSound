/*
Copyright (c) 2015, Dimitri Diakopoulos All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "Common.h"
#include <utility>
#include <map>
#include <memory>

namespace nqr
{
    
// Individual decoder classes will throw std::exceptions for bad things,
// but NyquistIO implements this enum for high-level error notifications.
enum IOError
{
    NoError,
    NoDecodersLoaded,
    ExtensionNotSupported,
    LoadPathNotImplemented,
    LoadBufferNotImplemented,
    UnknownError
};

struct BaseDecoder
{
    virtual int LoadFromPath(nqr::AudioData * data, const std::string & path) = 0;
    virtual int LoadFromBuffer(nqr::AudioData * data, const std::vector<uint8_t> & memory) = 0;
    virtual std::vector<std::string> GetSupportedFileExtensions() = 0;
    //@todo implement streaming helper methods here
};

    typedef std::pair<std::string, std::shared_ptr<nqr::BaseDecoder>> DecoderPair;

class NyquistIO
{
public:
    
    NyquistIO();
    ~NyquistIO();
    
    int Load(AudioData * data, const std::string & path);
    
    bool IsFileSupported(const std::string path) const;
    
private:
    
    std::string ParsePathForExtension(const std::string & path) const;
    
    std::shared_ptr<nqr::BaseDecoder> GetDecoderForExtension(const std::string ext);
    
    void BuildDecoderTable();
    
    template<typename T>
    void AddDecoderToTable(std::shared_ptr<T> decoder)
    {
        auto supportedExtensions = decoder->GetSupportedFileExtensions();
        
        //@todo: basic sanity checking that the extension isn't already supported
        for (const auto ext : supportedExtensions)
        {
            decoderTable.insert(DecoderPair(ext, decoder));
        }
    }
    
    std::map<std::string, std::shared_ptr<BaseDecoder>> decoderTable;
    
    NO_MOVE(NyquistIO);
};

} // end namespace nqr

#endif