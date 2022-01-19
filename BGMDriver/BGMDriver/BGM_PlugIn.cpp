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
//  BGM_PlugIn.cpp
//  BGMDriver
//
//  Copyright © 2016, 2017 Kyle Neideck
//  Copyright (C) 2013 Apple Inc. All Rights Reserved.
//  Copyright © 2020 MakeTheWeb
//
//  Based largely on SA_PlugIn.cpp from Apple's SimpleAudioDriver Plug-In sample code.
//  https://developer.apple.com/library/mac/samplecode/AudioDriverExamples
//

//	Self Include
#include "BGM_PlugIn.h"

//  Local Includes
#include "BGM_Device.h"
#include "BGM_NullDevice.h"

//  PublicUtility Includes
#include "CAException.h"
#include "CADebugMacros.h"
#include "CAPropertyAddress.h"
#include "CADispatchQueue.h"


#pragma mark Construction/Destruction

pthread_once_t				BGM_PlugIn::sStaticInitializer = PTHREAD_ONCE_INIT;
BGM_PlugIn*					BGM_PlugIn::sInstance = NULL;
AudioServerPlugInHostRef	BGM_PlugIn::sHost = NULL;

BGM_PlugIn& BGM_PlugIn::GetInstance()
{
    pthread_once(&sStaticInitializer, StaticInitializer);
    return *sInstance;
}

void	BGM_PlugIn::StaticInitializer()
{
    try
    {
        sInstance = new BGM_PlugIn;
        sInstance->Activate();
    }
    catch(...)
    {
        DebugMsg("BGM_PlugIn::StaticInitializer: failed to create the plug-in");
        delete sInstance;
        sInstance = NULL;
    }
}

BGM_PlugIn::BGM_PlugIn()
:
	BGM_Object(kAudioObjectPlugInObject, kAudioPlugInClassID, kAudioObjectClassID, 0),
	mMutex("BGM_PlugIn")
{
}

BGM_PlugIn::~BGM_PlugIn()
{
}

void	BGM_PlugIn::Deactivate()
{
	CAMutex::Locker theLocker(mMutex);
	BGM_Object::Deactivate();
    // TODO:
	//_RemoveAllDevices();
}

#pragma mark Property Operations

bool	BGM_PlugIn::HasProperty(AudioObjectID inObjectID, pid_t inClientPID, const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyManufacturer:
		case kAudioPlugInPropertyDeviceList:
		case kAudioPlugInPropertyTranslateUIDToDevice:
        case kAudioPlugInPropertyResourceBundle:
        case kAudioObjectPropertyCustomPropertyInfoList:
        case kAudioDeviceCustomPropertyBGMDeviceActive:
        case kAudioDeviceCustomPropertyBGMUISoundsDeviceActive:
        case kAudioPlugInCustomPropertyNullDeviceActive:
			theAnswer = true;
			break;
		
		default:
			theAnswer = BGM_Object::HasProperty(inObjectID, inClientPID, inAddress);
	};
	return theAnswer;
}

bool	BGM_PlugIn::IsPropertySettable(AudioObjectID inObjectID, pid_t inClientPID, const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyManufacturer:
		case kAudioPlugInPropertyDeviceList:
		case kAudioPlugInPropertyTranslateUIDToDevice:
        case kAudioPlugInPropertyResourceBundle:
        case kAudioObjectPropertyCustomPropertyInfoList:
			theAnswer = false;
			break;

        case kAudioDeviceCustomPropertyBGMDeviceActive:
        case kAudioDeviceCustomPropertyBGMUISoundsDeviceActive:
        case kAudioPlugInCustomPropertyNullDeviceActive:
            theAnswer = true;
            break;
		
		default:
			theAnswer = BGM_Object::IsPropertySettable(inObjectID, inClientPID, inAddress);
	};
	return theAnswer;
}

UInt32	BGM_PlugIn::GetPropertyDataSize(AudioObjectID inObjectID, pid_t inClientPID, const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyManufacturer:
			theAnswer = sizeof(CFStringRef);
			break;
			
		case kAudioObjectPropertyOwnedObjects:
		case kAudioPlugInPropertyDeviceList:
            // The plug-in owns the main BGM_Device, the instance of BGM_Device that handles UI
            // sounds and the null device.
            theAnswer = 0;

            if(BGM_Device::GetInstance().IsActive()) {
                theAnswer += sizeof(AudioObjectID);
            }

            if(BGM_Device::GetUISoundsInstance().IsActive()) {
                theAnswer += sizeof(AudioObjectID);
            }

            if(BGM_NullDevice::GetInstance().IsActive()) {
                theAnswer += sizeof(AudioObjectID);
            }

			break;
			
		case kAudioPlugInPropertyTranslateUIDToDevice:
			theAnswer = sizeof(AudioObjectID);
			break;
			
		case kAudioPlugInPropertyResourceBundle:
			theAnswer = sizeof(CFStringRef);
			break;

        case kAudioObjectPropertyCustomPropertyInfoList:
            theAnswer = 3 * sizeof(AudioServerPlugInCustomPropertyInfo);
            break;

        case kAudioDeviceCustomPropertyBGMDeviceActive:
        case kAudioDeviceCustomPropertyBGMUISoundsDeviceActive:
        case kAudioPlugInCustomPropertyNullDeviceActive:
            theAnswer = sizeof(CFBooleanRef);
            break;
		
		default:
			theAnswer = BGM_Object::GetPropertyDataSize(inObjectID, inClientPID, inAddress, inQualifierDataSize, inQualifierData);
	};
	return theAnswer;
}

void	BGM_PlugIn::GetPropertyData(AudioObjectID inObjectID, pid_t inClientPID, const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32& outDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyManufacturer:
			//	This is the human readable name of the maker of the plug-in.
			ThrowIf(inDataSize < sizeof(CFStringRef), CAException(kAudioHardwareBadPropertySizeError), "BGM_PlugIn::GetPropertyData: not enough space for the return value of kAudioObjectPropertyManufacturer");
			*reinterpret_cast<CFStringRef*>(outData) = CFSTR("Rewatch");
			outDataSize = sizeof(CFStringRef);
			break;
			
		case kAudioObjectPropertyOwnedObjects:
            // Fall through because this plug-in object only owns the devices.
		case kAudioPlugInPropertyDeviceList:
            {
#if DEBUG
                const char* thePropertyName =
                        inAddress.mSelector == kAudioObjectPropertyOwnedObjects ?
                                "kAudioObjectPropertyOwnedObjects" :
                                "kAudioPlugInPropertyDeviceList";
#endif
    			AudioObjectID* theReturnedDeviceList = reinterpret_cast<AudioObjectID*>(outData);
    			int theDeviceListIdx = 0;
                outDataSize = 0;

                if(BGM_Device::GetInstance().IsActive() &&
                        inDataSize >= outDataSize + sizeof(AudioObjectID))
                {
                    DebugMsg("BGM_PlugIn::GetPropertyData: Returning BGMDevice for %s",
                             thePropertyName);
                    theReturnedDeviceList[theDeviceListIdx++] = kObjectID_Device;
                    outDataSize += sizeof(AudioObjectID);
                }

                if(BGM_Device::GetUISoundsInstance().IsActive() &&
                        inDataSize >= outDataSize + sizeof(AudioObjectID))
                {
                    DebugMsg("BGM_PlugIn::GetPropertyData: Returning UI Sounds BGMDevice for %s",
                             thePropertyName);
                    theReturnedDeviceList[theDeviceListIdx++] = kObjectID_Device_UI_Sounds;
                    outDataSize += sizeof(AudioObjectID);
                }

                if(BGM_NullDevice::GetInstance().IsActive() &&
                        inDataSize >= outDataSize + sizeof(AudioObjectID))
                {
                    DebugMsg("BGM_PlugIn::GetPropertyData: Returning Null Device for %s",
                             thePropertyName);
                    theReturnedDeviceList[theDeviceListIdx++] = kObjectID_Device_Null;
                    outDataSize += sizeof(AudioObjectID);
                }
            }
			break;
			
		case kAudioPlugInPropertyTranslateUIDToDevice:
            {
                //	This property translates the UID passed in the qualifier as a CFString into the
                //	AudioObjectID for the device the UID refers to or kAudioObjectUnknown if no device
                //	has the UID.
                ThrowIf(inQualifierDataSize < sizeof(CFStringRef), CAException(kAudioHardwareBadPropertySizeError), "BGM_PlugIn::GetPropertyData: the qualifier size is too small for kAudioPlugInPropertyTranslateUIDToDevice");
                ThrowIf(inDataSize < sizeof(AudioObjectID), CAException(kAudioHardwareBadPropertySizeError), "BGM_PlugIn::GetPropertyData: not enough space for the return value of kAudioPlugInPropertyTranslateUIDToDevice");

                CFStringRef theUID = *reinterpret_cast<const CFStringRef*>(inQualifierData);
                AudioObjectID* outID = reinterpret_cast<AudioObjectID*>(outData);

                if(BGM_Device::GetInstance().IsActive() &&
                        CFEqual(theUID, BGM_Device::GetInstance().CopyDeviceUID()))
                {
                    DebugMsg("BGM_PlugIn::GetPropertyData: Returning BGMDevice for "
                             "kAudioPlugInPropertyTranslateUIDToDevice");
                    *outID = kObjectID_Device;
                }
                else if(BGM_Device::GetUISoundsInstance().IsActive() &&
                        CFEqual(theUID, BGM_Device::GetUISoundsInstance().CopyDeviceUID()))
                {
                    DebugMsg("BGM_PlugIn::GetPropertyData: Returning BGMUISoundsDevice for "
                             "kAudioPlugInPropertyTranslateUIDToDevice");
                    *outID = kObjectID_Device_UI_Sounds;
                }
                else if(BGM_NullDevice::GetInstance().IsActive() &&
                        CFEqual(theUID, BGM_NullDevice::GetInstance().CopyDeviceUID()))
                {
                    DebugMsg("BGM_PlugIn::GetPropertyData: Returning null device for "
                             "kAudioPlugInPropertyTranslateUIDToDevice");
                    *outID = kObjectID_Device_Null;
                }
                else
                {
                    LogWarning("BGM_PlugIn::GetPropertyData: Returning kAudioObjectUnknown for "
                               "kAudioPlugInPropertyTranslateUIDToDevice");
                    *outID = kAudioObjectUnknown;
                }

                outDataSize = sizeof(AudioObjectID);
            }
			break;
			
		case kAudioPlugInPropertyResourceBundle:
			//	The resource bundle is a path relative to the path of the plug-in's bundle.
			//	To specify that the plug-in bundle itself should be used, we just return the
			//	empty string.
			ThrowIf(inDataSize < sizeof(AudioObjectID), CAException(kAudioHardwareBadPropertySizeError), "BGM_GetPlugInPropertyData: not enough space for the return value of kAudioPlugInPropertyResourceBundle");
			*reinterpret_cast<CFStringRef*>(outData) = CFSTR("");
			outDataSize = sizeof(CFStringRef);
			break;

        case kAudioObjectPropertyCustomPropertyInfoList:
            {
                AudioServerPlugInCustomPropertyInfo* outCustomProperties =
                        reinterpret_cast<AudioServerPlugInCustomPropertyInfo*>(outData);

                if(inDataSize >= sizeof(AudioServerPlugInCustomPropertyInfo))
                {
                    outCustomProperties[0].mSelector = kAudioDeviceCustomPropertyBGMDeviceActive;
                    outCustomProperties[0].mPropertyDataType =
                            kAudioServerPlugInCustomPropertyDataTypeCFPropertyList;
                    outCustomProperties[0].mQualifierDataType =
                            kAudioServerPlugInCustomPropertyDataTypeNone;

                    outDataSize = sizeof(AudioServerPlugInCustomPropertyInfo);
                }
                else
                {
                    outDataSize = 0;
                }

                if(inDataSize >= sizeof(AudioServerPlugInCustomPropertyInfo) * 2)
                {
                    outCustomProperties[1].mSelector =
                            kAudioDeviceCustomPropertyBGMUISoundsDeviceActive;
                    outCustomProperties[1].mPropertyDataType =
                            kAudioServerPlugInCustomPropertyDataTypeCFPropertyList;
                    outCustomProperties[1].mQualifierDataType =
                            kAudioServerPlugInCustomPropertyDataTypeNone;

                    outDataSize = sizeof(AudioServerPlugInCustomPropertyInfo) * 2;
                }

                if(inDataSize >= sizeof(AudioServerPlugInCustomPropertyInfo) * 3)
                {
                    outCustomProperties[2].mSelector = kAudioPlugInCustomPropertyNullDeviceActive;
                    outCustomProperties[2].mPropertyDataType =
                        kAudioServerPlugInCustomPropertyDataTypeCFPropertyList;
                    outCustomProperties[2].mQualifierDataType =
                        kAudioServerPlugInCustomPropertyDataTypeNone;

                    outDataSize = sizeof(AudioServerPlugInCustomPropertyInfo) * 3;
                }
            }
            break;

        case kAudioDeviceCustomPropertyBGMDeviceActive:
            ThrowIf(inDataSize < sizeof(CFBooleanRef),
                    CAException(kAudioHardwareBadPropertySizeError),
                    "BGM_PlugIn::GetPropertyData: not enough space for the return value of "
                    "kAudioDeviceCustomPropertyBGMDeviceActive");
            *reinterpret_cast<CFBooleanRef*>(outData) =
                    BGM_Device::GetInstance().IsActive() ? kCFBooleanTrue : kCFBooleanFalse;
            outDataSize = sizeof(CFBooleanRef);
            break;

        case kAudioDeviceCustomPropertyBGMUISoundsDeviceActive:
            ThrowIf(inDataSize < sizeof(CFBooleanRef),
                    CAException(kAudioHardwareBadPropertySizeError),
                    "BGM_PlugIn::GetPropertyData: not enough space for the return value of "
                    "kAudioDeviceCustomPropertyBGMUISoundsDeviceActive");
            *reinterpret_cast<CFBooleanRef*>(outData) =
                    BGM_Device::GetUISoundsInstance().IsActive() ? kCFBooleanTrue : kCFBooleanFalse;
            outDataSize = sizeof(CFBooleanRef);
            break;

        case kAudioPlugInCustomPropertyNullDeviceActive:
            ThrowIf(inDataSize < sizeof(CFBooleanRef),
                    CAException(kAudioHardwareBadPropertySizeError),
                    "BGM_PlugIn::GetPropertyData: not enough space for the return value of "
                    "kAudioPlugInCustomPropertyNullDeviceActive");
            *reinterpret_cast<CFBooleanRef*>(outData) =
                BGM_NullDevice::GetInstance().IsActive() ? kCFBooleanTrue : kCFBooleanFalse;
            outDataSize = sizeof(CFBooleanRef);
            break;

		default:
			BGM_Object::GetPropertyData(inObjectID, inClientPID, inAddress, inQualifierDataSize, inQualifierData, inDataSize, outDataSize, outData);
			break;
	};
}

void	BGM_PlugIn::SetPropertyData(AudioObjectID inObjectID, pid_t inClientPID, const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData)
{
	switch(inAddress.mSelector)
	{
        case kAudioDeviceCustomPropertyBGMDeviceActive:
            SetDeviceActiveProperty(inDataSize, 
                                    inData,
                                    BGM_Device::GetInstance().IsActive(),
                                    []() { BGM_Device::GetInstance().Activate(); },
                                    []() { BGM_Device::GetInstance().Deactivate(); });
            break;

        case kAudioDeviceCustomPropertyBGMUISoundsDeviceActive:
            SetDeviceActiveProperty(inDataSize,
                                    inData,
                                    BGM_Device::GetUISoundsInstance().IsActive(),
                                    []() { BGM_Device::GetUISoundsInstance().Activate(); },
                                    []() { BGM_Device::GetUISoundsInstance().Deactivate(); });
            break;

        case kAudioPlugInCustomPropertyNullDeviceActive:
            SetDeviceActiveProperty(inDataSize,
                                    inData,
                                    BGM_NullDevice::GetInstance().IsActive(),
                                    []() { BGM_NullDevice::GetInstance().Activate(); },
                                    []() { BGM_NullDevice::GetInstance().Deactivate(); });
            break;
            
		default:
			BGM_Object::SetPropertyData(inObjectID, inClientPID, inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData);
			break;
	};
}

void	BGM_PlugIn::SetDeviceActiveProperty(UInt32 inDataSize,
                                            const void* inData,
                                            bool inDeviceIsActive,
                                            const std::function<void(void)>& inActivateDevice,
                                            const std::function<void(void)>& inDeactivateDevice)
{
    ThrowIf(inDataSize < sizeof(CFBooleanRef),
            CAException(kAudioHardwareBadPropertySizeError),
            "BGM_PlugIn::SetDeviceActiveProperty: wrong size for the data");

    CFBooleanRef theIsActiveRef = *reinterpret_cast<const CFBooleanRef*>(inData);

    ThrowIfNULL(theIsActiveRef,
                CAException(kAudioHardwareIllegalOperationError),
                "BGM_PlugIn::SetDeviceActiveProperty: null reference given");
    ThrowIf(CFGetTypeID(theIsActiveRef) != CFBooleanGetTypeID(),
            CAException(kAudioHardwareIllegalOperationError),
            "BGM_PlugIn::SetDeviceActiveProperty: CFType given for property was not a CFBoolean");

    bool theIsActive = CFBooleanGetValue(theIsActiveRef);

    // If the property isn't already set to the value given...
    if(theIsActive != inDeviceIsActive)
    {
        // Activate/deactivate the device.
        if(theIsActive)
        {
            inActivateDevice();
        }
        else
        {
            inDeactivateDevice();
        }

        // Send notifications.
        CADispatchQueue::GetGlobalSerialQueue().Dispatch(false, ^{
            AudioObjectPropertyAddress theChangedProperties[] = {
                    CAPropertyAddress(kAudioObjectPropertyOwnedObjects),
                    CAPropertyAddress(kAudioPlugInPropertyDeviceList)
            };

            Host_PropertiesChanged(GetObjectID(), 2, theChangedProperties);
        });
    }
}

