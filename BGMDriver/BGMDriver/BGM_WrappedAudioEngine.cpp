// This file is part of CleanShot Driver.
//
// CleanShot Driver is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 2 of the
// License, or (at your option) any later version.
//
// CleanShot Driver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CleanShot Driver. If not, see <http://www.gnu.org/licenses/>.

//
//  BGM_WrappedAudioEngine.cpp
//  BGMDriver
//
//  Copyright © 2016 Kyle Neideck
//

// Self Include
#include "BGM_WrappedAudioEngine.h"


// TODO: Register to be notified when the IO Registry values for these change so we can cache them

UInt64	BGM_WrappedAudioEngine::GetSampleRate() const
{
    return 0;
}

kern_return_t BGM_WrappedAudioEngine::SetSampleRate(Float64 inNewSampleRate)
{
    #pragma unused (inNewSampleRate)
    
    return 0;
}

UInt32 BGM_WrappedAudioEngine::GetSampleBufferFrameSize() const
{
    return 0;
}

