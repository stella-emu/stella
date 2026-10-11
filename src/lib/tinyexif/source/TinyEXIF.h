/*
  TinyEXIF.h -- A simple ISO C++ library to parse basic EXIF and XMP
                information from a JPEG file.

  Copyright (c) 2015-2025 Seacave
  cdc.seacave@gmail.com
  MIT License
*/

#ifndef __TINYEXIF_H__
#define __TINYEXIF_H__

#include <cstdint>
#include <string>
#include <vector>

#define TINYEXIF_MAJOR_VERSION 1
#define TINYEXIF_MINOR_VERSION 2
#define TINYEXIF_PATCH_VERSION 0

// TINYEXIF_VERSION_STRING and TINYEXIF_VERSION are derived from the three
// macros above -- change only MAJOR/MINOR/PATCH above, never these directly.
// CMakeLists.txt also reads MAJOR/MINOR/PATCH out of this header, so it is
// the single source of truth for the library version.
#define TINYEXIF_STRINGIZE_(x) #x
#define TINYEXIF_STRINGIZE(x) TINYEXIF_STRINGIZE_(x)
#define TINYEXIF_VERSION_STRING \
	TINYEXIF_STRINGIZE(TINYEXIF_MAJOR_VERSION) "." \
	TINYEXIF_STRINGIZE(TINYEXIF_MINOR_VERSION) "." \
	TINYEXIF_STRINGIZE(TINYEXIF_PATCH_VERSION)

// Numeric version for #if comparisons (macro values, not strings, are
// required in preprocessor conditions). Encodes MAJOR/MINOR/PATCH as
// MAJOR*10000 + MINOR*100 + PATCH, leaving room for two-digit MINOR/PATCH
// components. Example: gate use of an API added in 1.1.0 like this:
//   #if TINYEXIF_VERSION >= 10100
//   // use TinyEXIF::EXIFInfo::HasField(), added in 1.1.0
//   #endif
#define TINYEXIF_VERSION (TINYEXIF_MAJOR_VERSION*10000 + TINYEXIF_MINOR_VERSION*100 + TINYEXIF_PATCH_VERSION)

#ifdef _MSC_VER
#   ifdef TINYEXIF_EXPORT
#       define TINYEXIF_LIB __declspec(dllexport)
#   elif defined(TINYEXIF_IMPORT)
#       define TINYEXIF_LIB __declspec(dllimport)
#   else
#       define TINYEXIF_LIB
#   endif
#elif __GNUC__ >= 4
#   define TINYEXIF_LIB __attribute__((visibility("default")))
#else
#   define TINYEXIF_LIB
#endif

namespace TinyEXIF {

enum ErrorCode {
	PARSE_SUCCESS           = 0, // Parse EXIF and/or XMP was successful
	PARSE_INVALID_JPEG      = 1, // No JPEG markers found in buffer, possibly invalid JPEG file
	PARSE_UNKNOWN_BYTEALIGN = 2, // Byte alignment specified in EXIF file was unknown (neither Motorola nor Intel)
	PARSE_ABSENT_DATA       = 3, // No EXIF and/or XMP data found in JPEG file
	PARSE_CORRUPT_DATA      = 4, // EXIF and/or XMP header was found, but data was corrupted
};

enum FieldCode {
	FIELD_NA                 = 0, // No EXIF or XMP data
	FIELD_EXIF               = (1 << 0), // EXIF data available
	FIELD_XMP                = (1 << 1), // XMP data available
	FIELD_ALL                = FIELD_EXIF|FIELD_XMP
};

// Identifies one data field of EXIFInfo, for HasField()/GetFields(); it tells
// "tag absent" apart from "tag present and legitimately zero", which the value
// of a zero-initialized field alone can not.
// There is one enumerator per destination field, named FIELD_ID_ after the
// member it fills, not after the EXIF tag number: several EXIF tags and their
// XMP equivalents write the same member, and either source setting it counts
// as present. Note that FieldCode above (FIELD_EXIF/FIELD_XMP) is a different
// thing: it says which segment was found, not which tags were parsed.
// A floating point field only ever holds a finite value: a tag whose value is
// NaN or infinite, or converts to one, is treated as malformed and left absent.
// New enumerators are appended before FIELD_ID_COUNT, so the numeric value of
// an existing one never changes.
enum FieldID {
	// EXIFInfo
	FIELD_ID_ImageWidth = 0,
	FIELD_ID_ImageHeight,
	FIELD_ID_RelatedImageWidth,
	FIELD_ID_RelatedImageHeight,
	FIELD_ID_ImageDescription,
	FIELD_ID_Make,
	FIELD_ID_Model,
	FIELD_ID_SerialNumber,
	FIELD_ID_Orientation,
	FIELD_ID_XResolution,
	FIELD_ID_YResolution,
	FIELD_ID_ResolutionUnit,
	FIELD_ID_BitsPerSample,
	FIELD_ID_Software,
	FIELD_ID_DateTime,
	FIELD_ID_DateTimeOriginal,
	FIELD_ID_DateTimeDigitized,
	FIELD_ID_SubSecTimeOriginal,
	FIELD_ID_Copyright,
	FIELD_ID_ExposureTime,
	FIELD_ID_FNumber,
	FIELD_ID_ExposureProgram,
	FIELD_ID_ISOSpeedRatings,           // also set by the ExposureIndex tag, used interchangeably
	FIELD_ID_ShutterSpeedValue,
	FIELD_ID_ApertureValue,
	FIELD_ID_BrightnessValue,
	FIELD_ID_ExposureBiasValue,
	FIELD_ID_SubjectDistance,
	FIELD_ID_FocalLength,
	FIELD_ID_Flash,
	FIELD_ID_MeteringMode,
	FIELD_ID_LightSource,
	FIELD_ID_ProjectionType,            // the numeric value derived from GPano:ProjectionType
	FIELD_ID_SubjectArea,               // set if at least one of its components was read
	// EXIFInfo::Calibration
	FIELD_ID_Calibration_FocalLength,
	FIELD_ID_Calibration_OpticalCenterX,
	FIELD_ID_Calibration_OpticalCenterY,
	// EXIFInfo::Distortion
	FIELD_ID_Distortion_DewarpFlag,
	FIELD_ID_Distortion_K1,
	FIELD_ID_Distortion_K2,
	FIELD_ID_Distortion_P1,
	FIELD_ID_Distortion_P2,
	FIELD_ID_Distortion_K3,
	// EXIFInfo::LensInfo
	FIELD_ID_LensInfo_FStopMin,
	FIELD_ID_LensInfo_FStopMax,
	FIELD_ID_LensInfo_FocalLengthMin,
	FIELD_ID_LensInfo_FocalLengthMax,
	FIELD_ID_LensInfo_DigitalZoomRatio,
	FIELD_ID_LensInfo_FocalLengthIn35mm,
	FIELD_ID_LensInfo_FocalPlaneXResolution,
	FIELD_ID_LensInfo_FocalPlaneYResolution,
	FIELD_ID_LensInfo_FocalPlaneResolutionUnit,
	FIELD_ID_LensInfo_Make,
	FIELD_ID_LensInfo_Model,
	// EXIFInfo::GeoLocation
	FIELD_ID_GeoLocation_Latitude,      // set by the GPSLatitude components feeding LatComponents
	FIELD_ID_GeoLocation_Longitude,     // set by the GPSLongitude components feeding LonComponents
	FIELD_ID_GeoLocation_Altitude,
	FIELD_ID_GeoLocation_AltitudeRef,
	FIELD_ID_GeoLocation_RelativeAltitude,
	FIELD_ID_GeoLocation_RollDegree,
	FIELD_ID_GeoLocation_PitchDegree,
	FIELD_ID_GeoLocation_YawDegree,
	FIELD_ID_GeoLocation_SpeedX,
	FIELD_ID_GeoLocation_SpeedY,
	FIELD_ID_GeoLocation_SpeedZ,
	FIELD_ID_GeoLocation_AccuracyXY,
	FIELD_ID_GeoLocation_AccuracyZ,
	FIELD_ID_GeoLocation_GPSDOP,
	FIELD_ID_GeoLocation_GPSDifferential,
	FIELD_ID_GeoLocation_GPSMapDatum,
	FIELD_ID_GeoLocation_GPSTimeStamp,
	FIELD_ID_GeoLocation_GPSDateStamp,
	FIELD_ID_GeoLocation_LatComponents_direction,// GPSLatitudeRef, the N/S hemisphere of Latitude
	FIELD_ID_GeoLocation_LonComponents_direction,// GPSLongitudeRef, the E/W hemisphere of Longitude
	// EXIFInfo::GPano
	FIELD_ID_GPano_PosePitchDegrees,
	FIELD_ID_GPano_PoseRollDegrees,
	FIELD_ID_GPano_PoseHeadingDegrees,
	FIELD_ID_GPano_ProjectionType,
	FIELD_ID_GPano_CroppedAreaImageWidthPixels,
	FIELD_ID_GPano_CroppedAreaImageHeightPixels,
	FIELD_ID_GPano_FullPanoWidthPixels,
	FIELD_ID_GPano_FullPanoHeightPixels,
	FIELD_ID_GPano_CroppedAreaLeftPixels,
	FIELD_ID_GPano_CroppedAreaTopPixels,
	// EXIFInfo::MicroVideo
	FIELD_ID_MicroVideo_HasMicroVideo,
	FIELD_ID_MicroVideo_MicroVideoVersion,
	FIELD_ID_MicroVideo_MicroVideoOffset,
	FIELD_ID_MicroVideo_HasMotionPhoto,
	FIELD_ID_MicroVideo_MotionPhotoLength,
	FIELD_ID_MicroVideo_MotionPhotoMime,
	// added in 1.2.0
	FIELD_ID_MaxApertureValue,
	FIELD_ID_MPImages,                  // set if the MPF index listed at least one image
	FIELD_ID_COUNT                      // number of known fields; not a field itself
};

// Return the name of the given field as the path of the member it fills,
// e.g. "GeoLocation.Altitude"; empty string if the ID is not a known field.
TINYEXIF_LIB const char* FieldName(FieldID id);

class EntryParser;

//
// Interface class responsible for fetching stream data to be parsed
//
class TINYEXIF_LIB EXIFStream {
public:
	virtual ~EXIFStream() {}

	// Check the state of the stream.
	virtual bool IsValid() const = 0;

	// Return the pointer to the beginning of the desired size buffer
	// following current buffer position.
	virtual const uint8_t* GetBuffer(unsigned desiredLength) = 0;

	// Advance current buffer position with the desired size;
	// return false if stream ends in less than the desired size.
	virtual bool SkipBuffer(unsigned desiredLength) = 0;
};

//
// Class responsible for storing and parsing EXIF & XMP metadata from a JPEG stream
//
class TINYEXIF_LIB EXIFInfo {
public:
	EXIFInfo();
	EXIFInfo(EXIFStream& stream);
	EXIFInfo(std::istream& stream); // NB: the stream must have been opened in binary mode
	EXIFInfo(const uint8_t* data, unsigned length);

	// Parsing function for an entire JPEG image stream.
	//
	// PARAM 'stream': Interface to fetch JPEG image stream.
	// PARAM 'data': A pointer to a JPEG image.
	// PARAM 'length': The length of the JPEG image.
	// RETURN:  PARSE_SUCCESS (0) on success with 'result' filled out
	//          error code otherwise, as defined by the PARSE_* macros
	int parseFrom(EXIFStream& stream);
	int parseFrom(std::istream& stream); // NB: the stream must have been opened in binary mode
	int parseFrom(const uint8_t* data, unsigned length);

	// Parsing function for an EXIF segment. This is used internally by parseFrom()
	// but can be called for special cases where only the EXIF section is
	// available (i.e., a blob starting with the bytes "Exif\0\0").
	int parseFromEXIFSegment(const uint8_t* buf, unsigned len);

#ifndef TINYEXIF_NO_XMP_SUPPORT
	// Parsing function for an XMP segment. This is used internally by parseFrom()
	// but can be called for special cases where only the XMP section is
	// available (i.e., a blob starting with the bytes "http://ns.adobe.com/xap/1.0/\0").
	int parseFromXMPSegment(const uint8_t* buf, unsigned len);
	int parseFromXMPSegmentXML(const char* szXML, unsigned len);
#endif // TINYEXIF_NO_XMP_SUPPORT

	// Set all data members to default values.
	// Should be called before parsing a new stream.
	void clear();

	// Return true if the given field was actually present in the parsed data;
	// this is what distinguishes a tag that was absent from a tag that was
	// present and legitimately zero, which the field value alone can not.
	bool HasField(FieldID id) const;
	// Return the ID of every field that was found, in FieldID order;
	// see FieldName() to turn one into a printable name.
	std::vector<FieldID> GetFields() const;

private:
	// Parse tag as Image IFD; the sub-IFD offsets are 64bit as they are read from
	// attacker controlled data and must be range checked before being used.
	void parseIFDImage(EntryParser&, uint64_t&, uint64_t&);
	// Parse tag as Exif IFD.
	void parseIFDExif(EntryParser&);
	// Parse tag as GPS IFD.
	void parseIFDGPS(EntryParser&);
	// Parse tag as MakerNote IFD.
	void parseIFDMakerNote(EntryParser&);
	// Parse the MPF index, given the stream offset of its TIFF header.
	int parseFromMPFSegment(const uint8_t* buf, unsigned len, uint64_t offset);

	// Mark the given field as present.
	void SetField(FieldID id);
	// Mark the given field as present if 'fetched' says its fetch succeeded, and
	// return 'fetched' unchanged; every fetch site is instrumented through this,
	// so it neither hides which field it marks nor alters the control flow around it.
	bool SetFieldIf(FieldID id, bool fetched);

	// Presence bit per FieldID, queried by HasField(); a vector rather than a
	// fixed-size array so that adding fields later does not change this layout.
	std::vector<uint64_t> FieldsPresent;

public:
	// Data fields
	uint32_t Fields;                    // Store if EXIF and/or XMP data fields are available
	uint32_t ImageWidth;                // Image width reported in EXIF data
	uint32_t ImageHeight;               // Image height reported in EXIF data
	uint32_t RelatedImageWidth;         // Original image width reported in EXIF data
	uint32_t RelatedImageHeight;        // Original image height reported in EXIF data
	std::string ImageDescription;       // Image description
	std::string Make;                   // Camera manufacturer's name
	std::string Model;                  // Camera model
	std::string SerialNumber;           // Serial number of the body of the camera
	uint16_t Orientation;               // Image orientation, start of data corresponds to
									    // 0: unspecified in EXIF data
									    // 1: upper left of image
									    // 3: lower right of image
									    // 6: upper right of image
									    // 8: lower left of image
									    // 9: undefined
	double XResolution;                 // Number of pixels per ResolutionUnit in the ImageWidth direction
	double YResolution;                 // Number of pixels per ResolutionUnit in the ImageLength direction
	uint16_t ResolutionUnit;            // Unit of measurement for XResolution and YResolution
									    // 1: no absolute unit of measurement. Used for images that may have a non-square aspect ratio, but no meaningful absolute dimensions
									    // 2: inch
									    // 3: centimeter
	uint16_t BitsPerSample;             // Number of bits per component
	std::string Software;               // Software used
	std::string DateTime;               // File change date and time
	std::string DateTimeOriginal;       // Original file date and time (may not exist)
	std::string DateTimeDigitized;      // Digitization date and time (may not exist)
	std::string SubSecTimeOriginal;     // Sub-second time that original picture was taken
	std::string Copyright;              // File copyright information
	double ExposureTime;                // Exposure time in seconds
	double FNumber;                     // F/stop
	uint16_t ExposureProgram;           // Exposure program
	                                    // 0: not defined
	                                    // 1: manual
	                                    // 2: normal program
	                                    // 3: aperture priority
	                                    // 4: shutter priority
	                                    // 5: creative program
	                                    // 6: action program
	                                    // 7: portrait mode
	                                    // 8: landscape mode
	uint16_t ISOSpeedRatings;           // ISO speed
	double ShutterSpeedValue;           // Shutter speed, converted from APEX to an exposure time in seconds
	double ApertureValue;               // The lens aperture, converted from APEX to an f-number
	double MaxApertureValue;            // The widest aperture of the lens, as an f-number
	double BrightnessValue;             // The value of brightness
	double ExposureBiasValue;           // Exposure bias value in EV
	double SubjectDistance;             // Distance to focus point in meters
	double FocalLength;                 // Focal length of lens in millimeters
	uint16_t Flash;                     // Flash info
	                                    // Flash used (Flash&1)
	                                    // 0: no flash, >0: flash used
	                                    // Flash returned light status ((Flash & 6) >> 1)
	                                    // 0: no strobe return detection function
	                                    // 1: reserved
	                                    // 2: strobe return light not detected
	                                    // 3: strobe return light detected
	                                    // Flash mode ((Flash & 24) >> 3)
	                                    // 0: unknown
	                                    // 1: compulsory flash firing
	                                    // 2: compulsory flash suppression
	                                    // 3: auto mode
	                                    // Flash function ((Flash & 32) >> 5)
	                                    // 0: flash function present, >0: no flash function
	                                    // Flash red-eye ((Flash & 64) >> 6)
	                                    // 0: no red-eye reduction mode or unknown, >0: red-eye reduction supported
	uint16_t MeteringMode;              // Metering mode
	                                    // 0: unknown
	                                    // 1: average
	                                    // 2: center weighted average
	                                    // 3: spot
	                                    // 4: multi-spot
	                                    // 5: pattern
	                                    // 6: partial
	uint16_t LightSource;               // Kind of light source
	                                    // 0: unknown
	                                    // 1: daylight
	                                    // 2: fluorescent
	                                    // 3: tungsten (incandescent light)
	                                    // 4: flash
	                                    // 9: fine weather
	                                    // 10: cloudy weather
	                                    // 11: shade
	                                    // 12: daylight fluorescent (D 5700 - 7100K)
	                                    // 13: day white fluorescent (N 4600 - 5400K)
	                                    // 14: cool white fluorescent (W 3900 - 4500K)
	                                    // 15: white fluorescent (WW 3200 - 3700K)
	                                    // 17: standard light A
	                                    // 18: standard light B
	                                    // 19: standard light C
										// 20: D55
										// 21: D65
										// 22: D75
										// 23: D50
	                                    // 24: ISO studio tungsten
	uint16_t ProjectionType;            // Projection type
									    // 0: unknown projection
									    // 1: perspective projection
									    // 2: equirectangular/spherical projection
	std::vector<uint16_t> SubjectArea;  // Location and area of the main subject in the overall scene expressed in relation to the upper left as origin, prior to rotation
	                                    // 0: unknown
	                                    // 2: location of the main subject as coordinates (first value is the X coordinate and second is the Y coordinate)
	                                    // 3: area of the main subject as a circle (first value is the center X coordinate, second is the center Y coordinate, and third is the diameter)
	                                    // 4: area of the main subject as a rectangle (first value is the center X coordinate, second is the center Y coordinate, third is the width of the area, and fourth is the height of the area)
	struct TINYEXIF_LIB Calibration_t { // Camera calibration information
		double FocalLength;             // Focal length (pixels)
		double OpticalCenterX;          // Principal point X (pixels)
		double OpticalCenterY;          // Principal point Y (pixels)
		bool hasCalibration() const;    // Return true if FocalLength, OpticalCenterX, OpticalCenterY are available (nonzero)
	} Calibration;
	struct TINYEXIF_LIB Distortion_t { // Lens distortion information
		uint32_t DewarpFlag;            // Dewarp flag - indicates whether undistortion has been applied to the image
										// UINT32_MAX: flag missing from EXIF data
										// 0: no dewarp (raw image) - the image is distorted, should also have coefficients
										// 1: dewarp applied (undistorted image)
		double K1;
		double K2;
		double P1;
		double P2;
		double K3;
		bool hasDewarpFlag() const; // Return true if DewarpFlag is available
		bool hasDistortion() const; // Return true if any of K1, K2, P1, P2, K3 are available
	} Distortion;
	struct TINYEXIF_LIB LensInfo_t {    // Lens information
		double FStopMin;                // Widest aperture (f-number) at FocalLengthMin
		double FStopMax;                // Widest aperture (f-number) at FocalLengthMax
		double FocalLengthMin;          // Min focal length (mm)
		double FocalLengthMax;          // Max focal length (mm)
		double DigitalZoomRatio;        // Digital zoom ratio when the image was shot
		double FocalLengthIn35mm;       // Focal length in 35mm film
		double FocalPlaneXResolution;   // Number of pixels in the image width (X) direction per FocalPlaneResolutionUnit on the camera focal plane (may not exist)
		double FocalPlaneYResolution;   // Number of pixels in the image width (Y) direction per FocalPlaneResolutionUnit on the camera focal plane (may not exist)
		uint16_t FocalPlaneResolutionUnit;// Unit for measuring FocalPlaneXResolution and FocalPlaneYResolution (may not exist)
										// 0: unspecified in EXIF data
										// 1: no absolute unit of measurement
										// 2: inch
										// 3: centimeter
		std::string Make;               // Lens manufacturer
		std::string Model;              // Lens model
	} LensInfo;
	struct TINYEXIF_LIB Geolocation_t { // GPS information embedded in file
		double Latitude;                // Image latitude expressed as decimal
		double Longitude;               // Image longitude expressed as decimal
		double Altitude;                // Altitude in meters, relative to sea level
		int8_t AltitudeRef;             // 0: above sea level
										// 1: below sea level
										// 2: positive sea-level reference
										// 3: negative sea-level reference
		double RelativeAltitude;        // Relative altitude in meters
		double RollDegree;              // Flight roll in degrees
		double PitchDegree;             // Flight pitch in degrees
		double YawDegree;               // Flight yaw in degrees
		double SpeedX;                  // Flight speed on X in meters/second
		double SpeedY;                  // Flight speed on Y in meters/second
		double SpeedZ;                  // Flight speed on Z in meters/second
		double AccuracyXY;              // GPS accuracy on XY in meters
		double AccuracyZ;               // GPS accuracy on Z in meters
		double GPSDOP;                  // GPS DOP (data degree of precision)
		uint16_t GPSDifferential;       // Differential correction applied to the GPS receiver (may not exist)
										// 0: measurement without differential correction
										// 1: differential correction applied
		std::string GPSMapDatum;        // Geodetic survey data (may not exist)
		std::string GPSTimeStamp;       // Time as UTC (Coordinated Universal Time) (may not exist)
		std::string GPSDateStamp;       // A character string recording date and time information relative to UTC (Coordinated Universal Time) YYYY:MM:DD (may not exist)
		struct Coord_t {
			double degrees;
			double minutes;
			double seconds;
			uint8_t direction;
		} LatComponents, LonComponents; // Latitude/Longitude expressed in deg/min/sec
		void parseCoords();             // Convert Latitude/Longitude from deg/min/sec to decimal
		bool hasLatLon() const;         // Return true if (lat,lon) is available; not when the GPS had no fix (GPSStatus 'V')
		bool hasAltitude() const;       // Return true if (alt) is available
		bool hasRelativeAltitude()const;// Return true if (rel_alt) is available
		bool hasOrientation() const;    // Return true if (roll,yaw,pitch) is available
		bool hasSpeed() const;          // Return true if (speedX,speedY,speedZ) is available
		bool hasAccuracy() const;       // Return true if (accuracyXY,accuracyZ) is available
	} GeoLocation;
	struct TINYEXIF_LIB GPano_t {       // Spherical metadata. https://developers.google.com/streetview/spherical-metadata
		double PosePitchDegrees;        // Pitch, measured in degrees above the horizon, for the center in the image. Value must be >= -90 and <= 90.
		double PoseRollDegrees;         // Roll, measured in degrees, of the image where level with the horizon is 0. As roll increases, the horizon rotates counterclockwise in the image. Value must be > -180 and <= 180.
		double PoseHeadingDegrees;      // Compass heading, measured in degrees, for the center in the image. Value must be >= 0 and < 360.
		std::string ProjectionType;     // Type of image projection, e.g. "equirectangular" (raw spec string; see also EXIFInfo::ProjectionType for the derived numeric value)
		uint32_t CroppedAreaImageWidthPixels;// Width of the image after cropping to the panoramic field-of-view
		uint32_t CroppedAreaImageHeightPixels;// Height of the image after cropping to the panoramic field-of-view
		uint32_t FullPanoWidthPixels;   // Width of the full panorama the cropped image represents (may exceed CroppedAreaImageWidthPixels)
		uint32_t FullPanoHeightPixels;  // Height of the full panorama the cropped image represents (may exceed CroppedAreaImageHeightPixels)
		uint32_t CroppedAreaLeftPixels; // Column where the left edge of the cropped image was cropped from the full panorama
		uint32_t CroppedAreaTopPixels;  // Row where the top edge of the cropped image was cropped from the full panorama
		bool hasPosePitchDegrees() const; // Return true if PosePitchDegrees is available
		bool hasPoseRollDegrees() const; // Return true if PoseRollDegrees is available
		bool hasPoseHeadingDegrees() const; // Return true if PoseHeadingDegrees is available
		bool hasCroppedAreaImageWidthPixels() const; // Return true if CroppedAreaImageWidthPixels is available
		bool hasCroppedAreaImageHeightPixels() const; // Return true if CroppedAreaImageHeightPixels is available
		bool hasFullPanoWidthPixels() const; // Return true if FullPanoWidthPixels is available
		bool hasFullPanoHeightPixels() const; // Return true if FullPanoHeightPixels is available
		bool hasCroppedAreaLeftPixels() const; // Return true if CroppedAreaLeftPixels is available
		bool hasCroppedAreaTopPixels() const; // Return true if CroppedAreaTopPixels is available
		bool isEquirectangular() const; // Return true if ProjectionType is "equirectangular" or "spherical" (case-insensitive)
	} GPano;
	struct TINYEXIF_LIB MicroVideo_t {  // Google camera video file in metadata
		uint32_t HasMicroVideo;         // not zero if exists
		uint32_t MicroVideoVersion;     // just regularinfo
		uint32_t MicroVideoOffset;      // legacy GCamera:MicroVideo - offset from end of file
		uint32_t HasMotionPhoto;        // GCamera:MotionPhoto - not zero if exists (newer container format, supersedes GCamera:MicroVideo)
		uint32_t MotionPhotoLength;     // GCamera:MotionPhoto - length in bytes of the trailing video item, saturated at UINT32_MAX; a length, not an offset: it differs from MicroVideoOffset as soon as the container holds items after the video
		std::string MotionPhotoMime;    // GCamera:MotionPhoto - mime type of the trailing video item, e.g. "video/mp4" (empty if the container declared none)
	} MicroVideo;
	struct TINYEXIF_LIB MPImage_t {     // An image stored in the file, as listed by its Multi-Picture Format index (MPF, CIPA DC-007)
		uint32_t Type;                  // MP type code
		                                // 0x000000: undefined
		                                // 0x010001..0x010005: large thumbnail, a preview of the primary image (VGA, full HD, 4K, 8K, 16K equivalent)
		                                // 0x020001: multi-frame panorama
		                                // 0x020002: multi-frame disparity, one view of a stereo image
		                                // 0x020003: multi-frame multi-angle
		                                // 0x030000: baseline MP primary image
		                                // 0x040000: original preservation image
		                                // 0x050000: gain map image
		uint8_t Format;                 // Image data format; 0: JPEG, the only one defined
		uint8_t Flags;                  // Bit mask of
		                                // 4: representative image
		                                // 8: dependent child image
		                                // 16: dependent parent image
		uint32_t Length;                // Size of the image in bytes
		uint64_t Offset;                // Start of the image in bytes, from the start of the JPEG stream; 0 for the first image, normally the one this metadata belongs to
		bool isLargeThumbnail() const;  // Return true if this is a preview of the primary image (Type 0x01xxxx)
	};
	std::vector<MPImage_t> MPImages;    // Images listed by the MPF index, in its order (empty if there is none); they are only located, never read:
	                                    // Offset and Length come from the file, so check them against its size before reading the image
};

} // namespace TinyEXIF

#endif // __TINYEXIF_H__
