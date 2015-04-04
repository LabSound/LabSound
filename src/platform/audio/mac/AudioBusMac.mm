/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSoundConfig.h"

#import "AudioBus.h"

#import "AudioFileReader.h"
#import <Foundation/Foundation.h>

namespace WebCore {

std::unique_ptr<AudioBus> AudioBus::loadPlatformResource(const char* name, float sampleRate)
{

    NSBundle *bundle = [NSBundle mainBundle];
    /// @LabSound changed audio to HRTF
    NSURL *audioFileURL = [bundle URLForResource:[NSString stringWithUTF8String:name] withExtension:@"wav" subdirectory:@"HRTF"];

    /// @LabSound added read from $(CWD)/HRTF directory if bundle asset not available
    if (!audioFileURL) {
        NSString *path = [NSString stringWithFormat:@"HRTF/%s.wav", name];
        audioFileURL = [NSURL fileURLWithPath:path];
    }
    
    if (!audioFileURL) {
        return nullptr;
    }
    
    NSDataReadingOptions options = NSDataReadingMappedIfSafe;
    NSData *audioData = 0;

    @try {  // @LabSound added try/catch
        audioData = [NSData dataWithContentsOfURL:audioFileURL options:options error:nil];
    }
    @catch(NSException *exc) {
        return nullptr;
    }

    if (audioData) {
        return std::unique_ptr<AudioBus>(createBusFromInMemoryAudioFile([audioData bytes], [audioData length], false, sampleRate));
    }

    return nullptr;
}

} // namespace WebCore
