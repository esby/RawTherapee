/*
 *  This file is part of RawTherapee.
 */

#include <string.h>
#include "rtexif.h"

namespace rtexif
{

// TODO: write interpreters

class PanaSShortReader : public Interpreter
{
public:
    std::string toString (Tag* t) 
    {
        char buffer[32];
        int a = t->toInt (0, SSHORT); 
         sprintf (buffer,"%i",a);
        return buffer;
    }

   double toDouble (const Tag* t, int ofs )
    {
        return t->toInt (ofs, SSHORT);
    }


};
PanaSShortReader panaSShortReader;


class PanaOrientationInterpreter : public ChoiceInterpreter<int>
{
public:
    PanaOrientationInterpreter ()
    {
        choices[0] = "Horizontal (normal)";
        choices[1] = "Rotate CW";
        choices[2] = "Rotate 180";
        choices[3] = "Rotate CCW";
        choices[4] = "Tilt Upwards";
        choices[5] = "Tilt Downwards";
    }
 
     std::string toString (Tag* t) //override
    {
        int a = t->toInt (0, SHORT);
      
        printf("DEBUG: PanaOrientation choice= %i %s \n",a, choices[a].c_str());
        return choices[a];
    }

};
PanaOrientationInterpreter panaOrientationInterpreter;


const TagAttrib panasonicAttribs[] = {
    {0, AC_WRITE, 0, nullptr, 0x0001, AUTO, "Quality", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0002, AUTO, "FirmwareVersion", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0003, AUTO, "WhiteBalance", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0007, AUTO, "FocusMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x000f, AUTO, "AFMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x001a, AUTO, "ImageStabilization", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x001c, AUTO, "Macro", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x001f, AUTO, "ShootingMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0020, AUTO, "Audio", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0023, AUTO, "WhiteBalanceBias", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0024, AUTO, "FlashBias", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0025, AUTO, "InternalSerialNumber", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0026, AUTO, "ExifVersion", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0028, AUTO, "ColorEffect", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0029, AUTO, "TimeSincePowerOn", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x002a, AUTO, "BurstMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x002b, AUTO, "SequenceNumber", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x002c, AUTO, "Contrast", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x002d, AUTO, "NoiseReduction", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x002e, AUTO, "SelfTimer", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0030, AUTO, "Rotation", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0031, AUTO, "AFAssistLamp", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0032, AUTO, "ColorMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0033, AUTO, "BabyAge1", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0034, AUTO, "OpticalZoomMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0035, AUTO, "ConversionLens", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0036, AUTO, "TravelDay", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0039, AUTO, "Contrast", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x003a, AUTO, "WorldTimeLocation", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x003b, AUTO, "TextStamp1", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x003c, AUTO, "ProgramISO", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x003d, AUTO, "AdvancedSceneType", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x003e, AUTO, "TextStamp2", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x003f, AUTO, "FacesDetected", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0040, AUTO, "Saturation", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0041, AUTO, "Sharpness", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0042, AUTO, "FilmMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0044, AUTO, "ColorTempKelvin", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0045, AUTO, "BracketSettings", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0046, AUTO, "WBAdjustAB", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0047, AUTO, "WBAdjustGM", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0048, AUTO, "FlashCurtain", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0049, AUTO, "LongShutterNoiseReduction", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x004b, AUTO, "ImageWidth", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x004c, AUTO, "ImageHeight", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x004d, AUTO, "AFPointPosition", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x004e, AUTO, "FaceDetInfo", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0051, AUTO, "LensType", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0052, AUTO, "LensSerialNumber", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0053, AUTO, "AccessoryType", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0054, AUTO, "AccessorySerialNumber", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0059, AUTO, "Transform1", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x005d, AUTO, "IntelligentExposure", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0060, AUTO, "LensFirmwareVersion", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0061, AUTO, "FaceRecInfo", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0062, AUTO, "FlashWarning", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0065, AUTO, "Title", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0066, AUTO, "BabyName", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0067, AUTO, "Location", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0069, AUTO, "Country", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x006b, AUTO, "State", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x006d, AUTO, "City", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x006f, AUTO, "Landmark", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0070, AUTO, "IntelligentResolution", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0077, AUTO, "BurstSheed", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0079, AUTO, "IntelligentDRange", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x007c, AUTO, "ClearRetouch", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0080, AUTO, "City2", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0086, AUTO, "ManometerPressure", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0089, AUTO, "PhotoStyle", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x008a, AUTO, "ShadingCompensation", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x008c, AUTO, "AccelerometerZ", &panaSShortReader},
    {0, AC_WRITE, 0, nullptr, 0x008d, AUTO, "AccelerometerX", &panaSShortReader},
    {0, AC_WRITE, 0, nullptr, 0x008e, AUTO, "AccelerometerY", &panaSShortReader},
    {0, AC_WRITE, 0, nullptr, 0x008f, AUTO, "CameraOrientation", &panaOrientationInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0090, AUTO, "RollAngle", &panaSShortReader},
    {0, AC_WRITE, 0, nullptr, 0x0091, AUTO, "PitchAngle", &panaSShortReader},
    {0, AC_WRITE, 0, nullptr, 0x0093, AUTO, "SweepPanoramaDirection", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0094, AUTO, "PanoramaFieldOfView", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0096, AUTO, "TimerRecording", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x009d, AUTO, "InternalNDFilter", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x009e, AUTO, "HDR", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x009f, AUTO, "ShutterType", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x00a3, AUTO, "ClearRetouchValue", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x00ab, AUTO, "TouchAE", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0e00, AUTO, "PrintIM", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8000, AUTO, "MakerNoteVersion", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8001, AUTO, "SceneMode", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8004, AUTO, "WBRedLevel", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8005, AUTO, "WBGreenLevel", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8006, AUTO, "WBBlueLevel", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8007, AUTO, "FlashFired", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8008, AUTO, "TextStamp3", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8009, AUTO, "TextStamp4", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8010, AUTO, "BabyAge2", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x8012, AUTO, "Transform2", &stdInterpreter},
    { -1, AC_DONTWRITE, 0,  nullptr, 0, AUTO, "", nullptr }
};

const TagAttrib panasonicRawAttribs[] = {
    {0, AC_WRITE, 0, nullptr, 0x0001, AUTO, "Version", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0002, AUTO, "SensorWidth", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0003, AUTO, "SensorHeight", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0004, AUTO, "SensorTopBorder", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0005, AUTO, "SensorLeftBorder", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0006, AUTO, "ImageHeight", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0007, AUTO, "ImageWidth", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0011, AUTO, "RedBalance", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0012, AUTO, "BlueBalance", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0017, AUTO, "ISOSpeed", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0024, AUTO, "WBRedLevel", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0025, AUTO, "WBGreenLevel", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0026, AUTO, "WBBlueLevel", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x002e, AUTO, "PreviewImage", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x010f, AUTO, "Make", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0110, AUTO, "Model", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0111, AUTO, "StripOffsets", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0112, AUTO, "Orientation", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0116, AUTO, "RowsPerStrip", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0117, AUTO, "StripByteCounts", &stdInterpreter},
    {0, AC_WRITE, 0, nullptr, 0x0118, AUTO, "RawDataOffset", &stdInterpreter},
    { -1, AC_DONTWRITE, 0,  nullptr, 0, AUTO, "", nullptr }
};

}
