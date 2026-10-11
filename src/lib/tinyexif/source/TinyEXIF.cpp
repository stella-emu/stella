/*
  TinyEXIF.cpp -- A simple ISO C++ library to parse basic EXIF and XMP
                  information from a JPEG file.

  Copyright (c) 2015-2025 Seacave
  cdc.seacave@gmail.com
  MIT License
*/

#include "TinyEXIF.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cfloat>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

#ifndef TINYEXIF_NO_XMP_SUPPORT
#include <tinyxml2.h>
#endif // TINYEXIF_NO_XMP_SUPPORT

#ifdef _MSC_VER
namespace {
    int strcasecmp(const char* a, const char* b) {
        return _stricmp(a, b);
    }
}
#else
#include <string.h>
#endif


#ifndef TINYEXIF_NO_XMP_SUPPORT
// helpers used only by the XMP parser, so a build without it does not warn they are unused
namespace Tools {

	// search string inside a string, case sensitive
	static const char* strrnstr(const char* haystack, const char* needle, size_t len) {
		const size_t needle_len(strlen(needle));
		if (0 == needle_len)
			return haystack;
		if (len <= needle_len)
			return NULL;
		for (size_t i=len-needle_len; i-- > 0; ) {
			if (haystack[0] == needle[0] &&
				0 == strncmp(haystack, needle, needle_len))
				return haystack;
			haystack++;
		}
		return NULL;
	}

	// split an input string with a delimiter and fill a string vector
	static void strSplit(const std::string& str, char delim, std::vector<std::string>& values) {
		values.clear();
		std::string::size_type start(0), end(0);
		while (end != std::string::npos) {
			end = str.find(delim, start);
			values.emplace_back(str.substr(start, end-start));
			start = end + 1;
		}
	}

	// true if a number was parsed out of 'str' and only whitespace follows it, given the
	// 'end' where strtod() or strtoull() stopped: they parse the longest prefix they can,
	// so "12junk" would read as 12 and "1e/2" as 1/2 instead of being malformed
	static bool isWholeNumber(const char* str, const char* end) {
		if (end == str)
			return false;
		while (isspace((unsigned char)*end))
			++end;
		return *end == '\0';
	}

	// parse a decimal number out of untrusted XMP text; fails unless the whole text is
	// one number, surrounding whitespace aside, on a value out of range, and on the "nan"
	// and "inf" that strtod accepts as well, so that the result is always a finite number
	static bool strToDouble(const char* str, double& value) {
		char* end(NULL);
		errno = 0;
		const double parsed(strtod(str, &end));
		if (!isWholeNumber(str, end) || errno == ERANGE || !std::isfinite(parsed))
			return false;
		value = parsed;
		return true;
	}

	// make sure the given degrees value is between -180 and 180
	static double NormD180(double d) {
		return (d = fmod(d+180.0, 360.0)) < 0 ? d+180.0 : d-180.0;
	}

} // namespace Tools
#endif // TINYEXIF_NO_XMP_SUPPORT


namespace TinyEXIF {

enum JPEG_MARKERS {
	JM_START = 0xFF,
	JM_SOF0  = 0xC0,
	JM_SOF1  = 0xC1,
	JM_SOF2  = 0xC2,
	JM_SOF3  = 0xC3,
	JM_DHT   = 0xC4,
	JM_SOF5  = 0xC5,
	JM_SOF6  = 0xC6,
	JM_SOF7  = 0xC7,
	JM_JPG   = 0xC8,
	JM_SOF9  = 0xC9,
	JM_SOF10 = 0xCA,
	JM_SOF11 = 0xCB,
	JM_DAC   = 0xCC,
	JM_SOF13 = 0xCD,
	JM_SOF14 = 0xCE,
	JM_SOF15 = 0xCF,
	JM_RST0  = 0xD0,
	JM_RST1  = 0xD1,
	JM_RST2  = 0xD2,
	JM_RST3  = 0xD3,
	JM_RST4  = 0xD4,
	JM_RST5  = 0xD5,
	JM_RST6  = 0xD6,
	JM_RST7  = 0xD7,
	JM_SOI   = 0xD8,
	JM_EOI   = 0xD9,
	JM_SOS   = 0xDA,
	JM_DQT	 = 0xDB,
	JM_DNL	 = 0xDC,
	JM_DRI	 = 0xDD,
	JM_DHP	 = 0xDE,
	JM_EXP	 = 0xDF,
	JM_APP0	 = 0xE0,
	JM_APP1  = 0xE1, // EXIF and XMP
	JM_APP2  = 0xE2,
	JM_APP3  = 0xE3,
	JM_APP4  = 0xE4,
	JM_APP5  = 0xE5,
	JM_APP6  = 0xE6,
	JM_APP7  = 0xE7,
	JM_APP8  = 0xE8,
	JM_APP9  = 0xE9,
	JM_APP10 = 0xEA,
	JM_APP11 = 0xEB,
	JM_APP12 = 0xEC,
	JM_APP13 = 0xED, // IPTC
	JM_APP14 = 0xEE,
	JM_APP15 = 0xEF,
	JM_JPG0	 = 0xF0,
	JM_JPG1	 = 0xF1,
	JM_JPG2	 = 0xF2,
	JM_JPG3	 = 0xF3,
	JM_JPG4	 = 0xF4,
	JM_JPG5	 = 0xF5,
	JM_JPG6	 = 0xF6,
	JM_JPG7	 = 0xF7,
	JM_JPG8	 = 0xF8,
	JM_JPG9	 = 0xF9,
	JM_JPG10 = 0xFA,
	JM_JPG11 = 0xFB,
	JM_JPG12 = 0xFC,
	JM_JPG13 = 0xFD,
	JM_COM   = 0xFE
};


// Store 'value' in 'field' only if it is finite. Every floating point field must hold
// a finite value or none: inf and NaN from a crafted file would otherwise read back as
// present, since they differ from the DBL_MAX "absent" sentinel, and NaN slips through
// any range check a caller applies, as every comparison with it is false.
static bool AssignFinite(double& field, double value) {
	if (!std::isfinite(value))
		return false;
	field = value;
	return true;
}


// Parser helper
class EntryParser {
private:
	const uint8_t* buf;
	const unsigned len;
	const unsigned tiff_header_start;
	const bool alignIntel; // byte order of the file (TIFF header), independent of the host
	unsigned offs; // current offset into buffer
	uint16_t tag, format;
	uint32_t length;

public:
	EntryParser(const uint8_t* _buf, unsigned _len, unsigned _tiff_header_start, bool _alignIntel)
		: buf(_buf), len(_len), tiff_header_start(_tiff_header_start), alignIntel(_alignIntel), offs(0) {}

	// Every read of buf must be validated by this: does the range [offset, offset+size)
	// lie inside the buffer? Written as a subtraction so the check itself can not
	// overflow, and taking a 64bit offset so that offsets the caller computes from
	// attacker controlled data (base + idx*size) can not wrap around either.
	bool InBounds(uint64_t offset, uint32_t size) const {
		return offset <= len && (uint64_t)len - offset >= size;
	}

	void Init(unsigned _offs) {
		// ParseTag() steps forward by one entry before reading, so start one entry
		// earlier; the wrap-around for _offs < 12 is undone by that same step
		offs = _offs - 12;
	}

	// Step to the next 12-byte IFD entry and read it;
	// returns false without touching the current entry if it does not fit the buffer
	bool ParseTag() {
		offs  += 12;
		if (!InBounds(offs, 12))
			return false;
		tag    = parse16(buf + offs, alignIntel);
		format = parse16(buf + offs + 2, alignIntel);
		length = parse32(buf + offs + 4, alignIntel);
		return true;
	}

	const uint8_t* GetBuffer() const { return buf; }
	unsigned GetOffset() const { return offs; }
	bool IsIntelAligned() const { return alignIntel; }
	// a parser over the same buffer and TIFF header, reading in the given byte order
	EntryParser WithByteOrder(bool intel) const { return EntryParser(buf, len, tiff_header_start, intel); }

	uint16_t GetTag() const { return tag; }
	uint32_t GetLength() const { return length; }
	uint32_t GetData() const { return parse32(buf + offs + 8, alignIntel); }
	// absolute offset of the data this entry points to; 64bit because the data offset
	// is fully attacker controlled and would wrap when added to the header start
	uint64_t GetSubIFD() const { return (uint64_t)tiff_header_start + GetData(); }

	bool IsShort() const { return format == 3; }
	bool IsLong() const { return format == 4; }
	bool IsRational() const { return format == 5 || format == 10; }
	bool IsSRational() const { return format == 10; }
	bool IsFloat() const { return format == 11; }
	bool IsUndefined() const { return format == 7; }

	std::string FetchString() const {
		return parseString(buf, length, GetData(), tiff_header_start, len, alignIntel);
	}
	bool Fetch(std::string& val) const {
		if (format != 2 || length == 0)
			return false;
		val = FetchString();
		return true;
	}
	bool Fetch(uint8_t& val) const {
		if ((format != 1 && format != 2 && format != 6) || length == 0)
			return false;
		val = parse8(buf + offs + 8);
		return true;
	}
	bool Fetch(uint16_t& val) const {
		if (!IsShort() || length == 0)
			return false;
		val = parse16(buf + offs + 8, alignIntel);
		return true;
	}
	bool Fetch(uint16_t& val, uint32_t idx) const {
		if (!IsShort() || length <= idx)
			return false;
		const uint64_t offset = GetSubIFD() + (uint64_t)idx*2;
		if (!InBounds(offset, 2))
			return false;
		val = parse16(buf + (size_t)offset, alignIntel);
		return true;
	}
	bool Fetch(uint32_t& val) const {
		if (!IsLong() || length == 0)
			return false;
		val = parse32(buf + offs + 8, alignIntel);
		return true;
	}
	bool Fetch(float& val) const {
		if (!IsFloat() || length == 0)
			return false;
		// a FLOAT is raw IEEE 754 bits, so the file can encode inf or NaN directly
		const float value(parseFloat(buf + offs + 8, alignIntel));
		if (!std::isfinite(value))
			return false;
		val = value;
		return true;
	}
	bool Fetch(double& val) const {
		if (!IsRational() || length == 0)
			return false;
		const uint64_t offset = GetSubIFD();
		if (!InBounds(offset, 8))
			return false;
		val = parseRational(buf + (size_t)offset, alignIntel, IsSRational());
		return true;
	}
	bool Fetch(double& val, uint32_t idx) const {
		if (!IsRational() || length <= idx)
			return false;
		const uint64_t offset = GetSubIFD() + (uint64_t)idx*8;
		if (!InBounds(offset, 8))
			return false;
		val = parseRational(buf + (size_t)offset, alignIntel, IsSRational());
		return true;
	}

	// Fetch a value stored as type T into a member of a different type, e.g. an
	// image width written as SHORT instead of LONG; like Fetch(), it leaves 'val'
	// untouched when the entry is not a T. The temporary is initialized so that no
	// compiler has to prove Fetch() wrote it before it is read (gcc 16 can not, #30).
	template <typename T, typename V>
	bool FetchAs(V& val) const {
		T stored = T();
		if (!Fetch(stored))
			return false;
		val = (V)stored;
		return true;
	}

public:
	static uint8_t parse8(const uint8_t* buf) {
		return buf[0];
	}
	static uint16_t parse16(const uint8_t* buf, bool intel) {
		if (intel)
			return ((uint16_t)buf[1]<<8) | buf[0];
		return ((uint16_t)buf[0]<<8) | buf[1];
	}
	static uint32_t parse32(const uint8_t* buf, bool intel) {
		if (intel)
			return ((uint32_t)buf[3]<<24) |
				((uint32_t)buf[2]<<16) |
				((uint32_t)buf[1]<<8)  |
				buf[0];
		return ((uint32_t)buf[0]<<24) |
			((uint32_t)buf[1]<<16) |
			((uint32_t)buf[2]<<8)  |
			buf[3];
	}
	static float parseFloat(const uint8_t* buf, bool intel) {
		union {
			uint32_t i;
			float f;
		} i2f;
		i2f.i = parse32(buf, intel);
		return i2f.f;
	}
	static double parseRational(const uint8_t* buf, bool intel, bool isSigned) {
		const uint32_t denominator = parse32(buf+4, intel);
		if (denominator == 0)
			return 0.0;
		const uint32_t numerator = parse32(buf, intel);
		return isSigned ?
			(double)(int32_t)numerator/(double)(int32_t)denominator :
			(double)numerator/(double)denominator;
	}
	static std::string parseString(const uint8_t* buf,
		unsigned num_components,
		unsigned data,
		unsigned base,
		unsigned len,
		bool intel)
	{
		std::string value;
		// num_components is a raw attacker controlled count and FetchString() passes
		// it through unchecked, so reject zero here rather than at the callers: below,
		// num_components-1 is an unsigned expression that would wrap to 0xffffffff and
		// index that far past value.data(), then resize() to 4GiB if the byte read is 0
		if (num_components == 0)
			return value;
		if (num_components <= 4) {
			value.resize(num_components);
			char j = intel ? 0 : 24;
			char j_m = intel ? -8 : 8;
			for (unsigned i=0; i<num_components; ++i, j -= j_m)
				value[i] = (data >> j) & 0xff;
			if (value[num_components-1] == '\0')
				value.resize(num_components-1);
		} else
		if ((uint64_t)base+data+num_components <= (uint64_t)len) {
			const char* const sz((const char*)buf+base+data);
			unsigned num(0);
			while (num < num_components && sz[num] != '\0')
				++num;
			while (num && sz[num-1] == ' ')
				--num;
			value.assign(sz, num);
		}
		return value;
	}
};


// Constructors
EXIFInfo::EXIFInfo() : Fields(FIELD_NA) {
}
EXIFInfo::EXIFInfo(EXIFStream& stream) {
	parseFrom(stream);
}
EXIFInfo::EXIFInfo(std::istream& stream) {
	parseFrom(stream);
}
EXIFInfo::EXIFInfo(const uint8_t* data, unsigned length) {
	parseFrom(data, length);
}


// Number of 64bit words needed to store one presence bit per FieldID
static const size_t FIELD_ID_WORDS = ((size_t)FIELD_ID_COUNT + 63) / 64;

// Name of every field, in FieldID order: the path of the member it fills
static const char* const g_FieldNames[] = {
	"ImageWidth",
	"ImageHeight",
	"RelatedImageWidth",
	"RelatedImageHeight",
	"ImageDescription",
	"Make",
	"Model",
	"SerialNumber",
	"Orientation",
	"XResolution",
	"YResolution",
	"ResolutionUnit",
	"BitsPerSample",
	"Software",
	"DateTime",
	"DateTimeOriginal",
	"DateTimeDigitized",
	"SubSecTimeOriginal",
	"Copyright",
	"ExposureTime",
	"FNumber",
	"ExposureProgram",
	"ISOSpeedRatings",
	"ShutterSpeedValue",
	"ApertureValue",
	"BrightnessValue",
	"ExposureBiasValue",
	"SubjectDistance",
	"FocalLength",
	"Flash",
	"MeteringMode",
	"LightSource",
	"ProjectionType",
	"SubjectArea",
	"Calibration.FocalLength",
	"Calibration.OpticalCenterX",
	"Calibration.OpticalCenterY",
	"Distortion.DewarpFlag",
	"Distortion.K1",
	"Distortion.K2",
	"Distortion.P1",
	"Distortion.P2",
	"Distortion.K3",
	"LensInfo.FStopMin",
	"LensInfo.FStopMax",
	"LensInfo.FocalLengthMin",
	"LensInfo.FocalLengthMax",
	"LensInfo.DigitalZoomRatio",
	"LensInfo.FocalLengthIn35mm",
	"LensInfo.FocalPlaneXResolution",
	"LensInfo.FocalPlaneYResolution",
	"LensInfo.FocalPlaneResolutionUnit",
	"LensInfo.Make",
	"LensInfo.Model",
	"GeoLocation.Latitude",
	"GeoLocation.Longitude",
	"GeoLocation.Altitude",
	"GeoLocation.AltitudeRef",
	"GeoLocation.RelativeAltitude",
	"GeoLocation.RollDegree",
	"GeoLocation.PitchDegree",
	"GeoLocation.YawDegree",
	"GeoLocation.SpeedX",
	"GeoLocation.SpeedY",
	"GeoLocation.SpeedZ",
	"GeoLocation.AccuracyXY",
	"GeoLocation.AccuracyZ",
	"GeoLocation.GPSDOP",
	"GeoLocation.GPSDifferential",
	"GeoLocation.GPSMapDatum",
	"GeoLocation.GPSTimeStamp",
	"GeoLocation.GPSDateStamp",
	"GeoLocation.LatComponents.direction",
	"GeoLocation.LonComponents.direction",
	"GPano.PosePitchDegrees",
	"GPano.PoseRollDegrees",
	"GPano.PoseHeadingDegrees",
	"GPano.ProjectionType",
	"GPano.CroppedAreaImageWidthPixels",
	"GPano.CroppedAreaImageHeightPixels",
	"GPano.FullPanoWidthPixels",
	"GPano.FullPanoHeightPixels",
	"GPano.CroppedAreaLeftPixels",
	"GPano.CroppedAreaTopPixels",
	"MicroVideo.HasMicroVideo",
	"MicroVideo.MicroVideoVersion",
	"MicroVideo.MicroVideoOffset",
	"MicroVideo.HasMotionPhoto",
	"MicroVideo.MotionPhotoLength",
	"MicroVideo.MotionPhotoMime",
	"MaxApertureValue",
	"MPImages",
};
static_assert(sizeof(g_FieldNames)/sizeof(g_FieldNames[0]) == (size_t)FIELD_ID_COUNT,
	"g_FieldNames must have exactly one entry per FieldID");

const char* FieldName(FieldID id) {
	const unsigned pos((unsigned)id);
	return pos < (unsigned)FIELD_ID_COUNT ? g_FieldNames[pos] : "";
}

bool EXIFInfo::HasField(FieldID id) const {
	const unsigned pos((unsigned)id);
	if (pos >= (unsigned)FIELD_ID_COUNT)
		return false;
	const size_t word(pos/64);
	// the bitset is only allocated once something is stored in it, so a parse
	// that found nothing at all (or no parse at all) leaves it empty
	return word < FieldsPresent.size() &&
		(FieldsPresent[word] & ((uint64_t)1 << (pos%64))) != 0;
}

std::vector<FieldID> EXIFInfo::GetFields() const {
	std::vector<FieldID> fields;
	for (unsigned pos=0; pos<(unsigned)FIELD_ID_COUNT; ++pos)
		if (HasField((FieldID)pos))
			fields.push_back((FieldID)pos);
	return fields;
}

void EXIFInfo::SetField(FieldID id) {
	const unsigned pos((unsigned)id);
	if (pos >= (unsigned)FIELD_ID_COUNT)
		return;
	// parseFromEXIFSegment()/parseFromXMPSegment() may be called directly, without
	// the clear() that parseFrom() does, so the storage is allocated on demand
	if (FieldsPresent.size() < FIELD_ID_WORDS)
		FieldsPresent.resize(FIELD_ID_WORDS, 0);
	FieldsPresent[pos/64] |= (uint64_t)1 << (pos%64);
}

bool EXIFInfo::SetFieldIf(FieldID id, bool fetched) {
	if (fetched)
		SetField(id);
	return fetched;
}


// Parse tag as Image IFD
void EXIFInfo::parseIFDImage(EntryParser& parser, uint64_t& exif_sub_ifd_offset, uint64_t& gps_sub_ifd_offset) {
	switch (parser.GetTag()) {
	case 0x0102:
		// Bits per sample
		SetFieldIf(FIELD_ID_BitsPerSample, parser.Fetch(BitsPerSample));
		break;

	case 0x010e:
		// Image description
		SetFieldIf(FIELD_ID_ImageDescription, parser.Fetch(ImageDescription));
		break;

	case 0x010f:
		// Camera maker
		SetFieldIf(FIELD_ID_Make, parser.Fetch(Make));
		break;

	case 0x0110:
		// Camera model
		SetFieldIf(FIELD_ID_Model, parser.Fetch(Model));
		break;

	case 0x0112:
		// Orientation of image
		SetFieldIf(FIELD_ID_Orientation, parser.Fetch(Orientation));
		break;

	case 0x011a:
		// XResolution 
		SetFieldIf(FIELD_ID_XResolution, parser.Fetch(XResolution));
		break;

	case 0x011b:
		// YResolution 
		SetFieldIf(FIELD_ID_YResolution, parser.Fetch(YResolution));
		break;

	case 0x0128:
		// Resolution Unit
		SetFieldIf(FIELD_ID_ResolutionUnit, parser.Fetch(ResolutionUnit));
		break;

	case 0x0131:
		// Software used for image
		SetFieldIf(FIELD_ID_Software, parser.Fetch(Software));
		break;

	case 0x0132:
		// EXIF/TIFF date/time of image modification
		SetFieldIf(FIELD_ID_DateTime, parser.Fetch(DateTime));
		break;

	case 0x1001:
		// Original Image width (LONG or SHORT)
		SetFieldIf(FIELD_ID_RelatedImageWidth, parser.Fetch(RelatedImageWidth) || parser.FetchAs<uint16_t>(RelatedImageWidth));
		break;

	case 0x1002:
		// Original Image height (LONG or SHORT)
		SetFieldIf(FIELD_ID_RelatedImageHeight, parser.Fetch(RelatedImageHeight) || parser.FetchAs<uint16_t>(RelatedImageHeight));
		break;

	case 0x8298:
		// Copyright information
		SetFieldIf(FIELD_ID_Copyright, parser.Fetch(Copyright));
		break;

	case 0x8769:
		// EXIF SubIFD offset
		exif_sub_ifd_offset = parser.GetSubIFD();
		break;

	case 0x8825:
		// GPS IFS offset
		gps_sub_ifd_offset = parser.GetSubIFD();
		break;

	default:
		// Try to parse as EXIF tag, as some images store them in here
		parseIFDExif(parser);
		break;
	}
}

// Parse tag as Exif IFD
void EXIFInfo::parseIFDExif(EntryParser& parser) {
	switch (parser.GetTag()) {
	case 0x02bc:
#ifndef TINYEXIF_NO_XMP_SUPPORT
		// XMP Metadata (Adobe technote 9-14-02)
		if (parser.IsUndefined()) {
			const std::string strXML(parser.FetchString());
			parseFromXMPSegmentXML(strXML.c_str(), (unsigned)strXML.length());
		}
#endif // TINYEXIF_NO_XMP_SUPPORT
		break;

	case 0x829a:
		// Exposure time in seconds
		SetFieldIf(FIELD_ID_ExposureTime, parser.Fetch(ExposureTime));
		break;

	case 0x829d:
		// FNumber
		SetFieldIf(FIELD_ID_FNumber, parser.Fetch(FNumber));
		break;

	case 0x8822:
		// Exposure Program
		SetFieldIf(FIELD_ID_ExposureProgram, parser.Fetch(ExposureProgram));
		break;

	case 0x8827:
		// ISO Speed Rating
		SetFieldIf(FIELD_ID_ISOSpeedRatings, parser.Fetch(ISOSpeedRatings));
		break;

	case 0x9003:
		// Original date and time
		SetFieldIf(FIELD_ID_DateTimeOriginal, parser.Fetch(DateTimeOriginal));
		break;

	case 0x9004:
		// Digitization date and time
		SetFieldIf(FIELD_ID_DateTimeDigitized, parser.Fetch(DateTimeDigitized));
		break;

	case 0x9201: {
		// Shutter speed value, converted from APEX to seconds
		// the conversion only runs on a value that was really fetched: applied to the
		// untouched 0 it would yield a plausible 1s; and its result is only kept when
		// finite: an APEX value far outside any camera's range underflows exp() to 0
		double apex(0);
		SetFieldIf(FIELD_ID_ShutterSpeedValue, parser.Fetch(apex) && AssignFinite(ShutterSpeedValue, 1.0/exp(apex*log(2))));
		break;
	}

	case 0x9202: {
		// Aperture value, converted from APEX to an f-number
		// as above: the untouched 0 would convert to a plausible f/1, and an APEX
		// value far outside any lens's range overflows exp() to inf
		double apex(0);
		SetFieldIf(FIELD_ID_ApertureValue, parser.Fetch(apex) && AssignFinite(ApertureValue, exp(apex*log(2)*0.5)));
		break;
	}

	case 0x9205: {
		// Max aperture value, the widest aperture of the lens, converted from APEX
		// to an f-number exactly like ApertureValue above
		double apex(0);
		SetFieldIf(FIELD_ID_MaxApertureValue, parser.Fetch(apex) && AssignFinite(MaxApertureValue, exp(apex*log(2)*0.5)));
		break;
	}

	case 0x9203:
		// Brightness value
		SetFieldIf(FIELD_ID_BrightnessValue, parser.Fetch(BrightnessValue));
		break;

	case 0x9204:
		// Exposure bias value 
		SetFieldIf(FIELD_ID_ExposureBiasValue, parser.Fetch(ExposureBiasValue));
		break;

	case 0x9206:
		// Subject distance
		SetFieldIf(FIELD_ID_SubjectDistance, parser.Fetch(SubjectDistance));
		break;

	case 0x9207:
		// Metering mode
		SetFieldIf(FIELD_ID_MeteringMode, parser.Fetch(MeteringMode));
		break;

	case 0x9208:
		// Light source
		SetFieldIf(FIELD_ID_LightSource, parser.Fetch(LightSource));
		break;

	case 0x9209:
		// Flash info
		SetFieldIf(FIELD_ID_Flash, parser.Fetch(Flash));
		break;

	case 0x920a:
		// Focal length
		SetFieldIf(FIELD_ID_FocalLength, parser.Fetch(FocalLength));
		break;

	case 0x9214:
		// Subject area
		if (parser.IsShort() && parser.GetLength() > 1) {
			// GetLength() is an attacker-controlled count; validate the whole array
			// fits the buffer before sizing the vector to match it, rather than
			// resizing first and relying on the per-element Fetch() bounds check
			const uint64_t dataSize = (uint64_t)parser.GetLength()*2;
			if (dataSize <= UINT32_MAX && parser.InBounds(parser.GetSubIFD(), (uint32_t)dataSize)) {
				SubjectArea.resize(parser.GetLength());
				for (uint32_t i=0; i<parser.GetLength(); ++i)
					SetFieldIf(FIELD_ID_SubjectArea, parser.Fetch(SubjectArea[i], i));
			}
		}
		break;

	case 0x927c:
		// MakerNote
		parseIFDMakerNote(parser);
		break;

	case 0x9291:
		// Fractions of seconds for DateTimeOriginal
		SetFieldIf(FIELD_ID_SubSecTimeOriginal, parser.Fetch(SubSecTimeOriginal));
		break;

	case 0xa002:
		// EXIF Image width (LONG or SHORT)
		SetFieldIf(FIELD_ID_ImageWidth, parser.Fetch(ImageWidth) || parser.FetchAs<uint16_t>(ImageWidth));
		break;

	case 0xa003:
		// EXIF Image height (LONG or SHORT)
		SetFieldIf(FIELD_ID_ImageHeight, parser.Fetch(ImageHeight) || parser.FetchAs<uint16_t>(ImageHeight));
		break;

	case 0xa20e:
		// Focal plane X resolution
		SetFieldIf(FIELD_ID_LensInfo_FocalPlaneXResolution, parser.Fetch(LensInfo.FocalPlaneXResolution));
		break;

	case 0xa20f:
		// Focal plane Y resolution
		SetFieldIf(FIELD_ID_LensInfo_FocalPlaneYResolution, parser.Fetch(LensInfo.FocalPlaneYResolution));
		break;

	case 0xa210:
		// Focal plane resolution unit
		SetFieldIf(FIELD_ID_LensInfo_FocalPlaneResolutionUnit, parser.Fetch(LensInfo.FocalPlaneResolutionUnit));
		break;

	case 0xa215:
		// Exposure Index and ISO Speed Rating are often used interchangeably;
		// converting a double that uint16_t can not represent is undefined behavior,
		// so a negative index is rejected and a large one clamped, as EXIF does for ISO
		if (ISOSpeedRatings == 0) {
			double ExposureIndex(0);
			if (parser.Fetch(ExposureIndex) && ExposureIndex >= 0) {
				ISOSpeedRatings = (uint16_t)std::min(ExposureIndex, (double)UINT16_MAX);
				SetField(FIELD_ID_ISOSpeedRatings);
			}
		}
		break;

	case 0xa404:
		// Digital Zoom Ratio
		SetFieldIf(FIELD_ID_LensInfo_DigitalZoomRatio, parser.Fetch(LensInfo.DigitalZoomRatio));
		break;

	case 0xa405:
		// Focal length in 35mm film (SHORT per EXIF, RATIONAL in some files)
		SetFieldIf(FIELD_ID_LensInfo_FocalLengthIn35mm, parser.Fetch(LensInfo.FocalLengthIn35mm) || parser.FetchAs<uint16_t>(LensInfo.FocalLengthIn35mm));
		break;

	case 0xa431:
		// Serial number of the camera
		SetFieldIf(FIELD_ID_SerialNumber, parser.Fetch(SerialNumber));
		break;

	case 0xa432:
		// Focal length and FStop.
		if (SetFieldIf(FIELD_ID_LensInfo_FocalLengthMin, parser.Fetch(LensInfo.FocalLengthMin, 0)))
			if (SetFieldIf(FIELD_ID_LensInfo_FocalLengthMax, parser.Fetch(LensInfo.FocalLengthMax, 1)))
				if (SetFieldIf(FIELD_ID_LensInfo_FStopMin, parser.Fetch(LensInfo.FStopMin, 2)))
					SetFieldIf(FIELD_ID_LensInfo_FStopMax, parser.Fetch(LensInfo.FStopMax, 3));
		break;

	case 0xa433:
		// Lens make.
		SetFieldIf(FIELD_ID_LensInfo_Make, parser.Fetch(LensInfo.Make));
		break;

	case 0xa434:
		// Lens model.
		SetFieldIf(FIELD_ID_LensInfo_Model, parser.Fetch(LensInfo.Model));
		break;
	}
}

// Parse tag as MakerNote IFD
void EXIFInfo::parseIFDMakerNote(EntryParser& exif) {
	if (0 != strcasecmp(Make.c_str(), "DJI"))
		return;
	// the MakerNote is a full IFD of its own: entry count followed by 12-byte entries,
	// none of which the tag's own length field is allowed to vouch for
	const uint64_t off = exif.GetSubIFD();
	if (!exif.InBounds(off, 4))
		return;
	// it is walked by a parser of its own, as its byte order can differ from the EXIF
	// one: DJI writes it little-endian, and a tool rewriting the EXIF in Motorola order
	// copies this opaque blob unchanged; the right order is the one in which the first
	// entry reads as tag 1, the "DJI" make
	const uint8_t* const note(exif.GetBuffer() + (size_t)off);
	EntryParser parser(exif.WithByteOrder(EntryParser::parse16(note + 2, true) == 1));
	int num_entries = EntryParser::parse16(note, parser.IsIntelAligned());
	if (uint32_t(2 + 12 * num_entries) > exif.GetLength())
		return;
	if (!parser.InBounds(off+2, 12*(uint32_t)num_entries))
		return;
	parser.Init((unsigned)(off+2));
	if (!parser.ParseTag())
		return;
	--num_entries;
	std::string maker;
	if (parser.GetTag() == 1 && parser.Fetch(maker)) {
		if (0 == strcasecmp(maker.c_str(), "DJI")) {
			while (--num_entries >= 0) {
				if (!parser.ParseTag())
					break;
				switch (parser.GetTag()) {
				case 3:
					// SpeedX
					SetFieldIf(FIELD_ID_GeoLocation_SpeedX, parser.FetchAs<float>(GeoLocation.SpeedX));
					break;

				case 4:
					// SpeedY
					SetFieldIf(FIELD_ID_GeoLocation_SpeedY, parser.FetchAs<float>(GeoLocation.SpeedY));
					break;

				case 5:
					// SpeedZ
					SetFieldIf(FIELD_ID_GeoLocation_SpeedZ, parser.FetchAs<float>(GeoLocation.SpeedZ));
					break;

				case 9:
					// Camera Pitch
					SetFieldIf(FIELD_ID_GeoLocation_PitchDegree, parser.FetchAs<float>(GeoLocation.PitchDegree));
					break;

				case 10:
					// Camera Yaw
					SetFieldIf(FIELD_ID_GeoLocation_YawDegree, parser.FetchAs<float>(GeoLocation.YawDegree));
					break;

				case 11:
					// Camera Roll
					SetFieldIf(FIELD_ID_GeoLocation_RollDegree, parser.FetchAs<float>(GeoLocation.RollDegree));
					break;
				}
			}
		}
	}
}

// GPS IFD tags 1 to 6: latitude, longitude and altitude, each with its reference
static bool IsGPSPositionTag(uint16_t tag) {
	return tag >= 1 && tag <= 6;
}

// True if the GPS IFD whose entries start at 'offs' has GPSStatus 'V': the receiver
// reports its measurement void, i.e. it had no fix. Cameras still write the position
// tags then, usually all zeros, which would place the image at 0N 0E; so the caller
// skips them, leaving the position absent instead of wrong.
static bool IsGPSMeasurementVoid(EntryParser& parser, unsigned offs, unsigned num_entries) {
	std::string status;
	parser.Init(offs);
	while (num_entries-- > 0 && parser.ParseTag())
		if (parser.GetTag() == 9 && parser.Fetch(status))
			return status == "V";
	return false;
}

// Parse tag as GPS IFD
void EXIFInfo::parseIFDGPS(EntryParser& parser) {
	switch (parser.GetTag()) {
	case 1:
		// GPS north or south
		SetFieldIf(FIELD_ID_GeoLocation_LatComponents_direction, parser.Fetch(GeoLocation.LatComponents.direction));
		break;

	case 2:
		// GPS latitude
		// the components are what parseCoords() turns into GeoLocation.Latitude,
		// so they mark that field rather than one enumerator each
		if (parser.IsRational() && parser.GetLength() == 3) {
			SetFieldIf(FIELD_ID_GeoLocation_Latitude, parser.Fetch(GeoLocation.LatComponents.degrees, 0));
			SetFieldIf(FIELD_ID_GeoLocation_Latitude, parser.Fetch(GeoLocation.LatComponents.minutes, 1));
			SetFieldIf(FIELD_ID_GeoLocation_Latitude, parser.Fetch(GeoLocation.LatComponents.seconds, 2));
		}
		break;

	case 3:
		// GPS east or west
		SetFieldIf(FIELD_ID_GeoLocation_LonComponents_direction, parser.Fetch(GeoLocation.LonComponents.direction));
		break;

	case 4:
		// GPS longitude
		if (parser.IsRational() && parser.GetLength() == 3) {
			SetFieldIf(FIELD_ID_GeoLocation_Longitude, parser.Fetch(GeoLocation.LonComponents.degrees, 0));
			SetFieldIf(FIELD_ID_GeoLocation_Longitude, parser.Fetch(GeoLocation.LonComponents.minutes, 1));
			SetFieldIf(FIELD_ID_GeoLocation_Longitude, parser.Fetch(GeoLocation.LonComponents.seconds, 2));
		}
		break;

	case 5:
		// GPS altitude reference (below or above sea level)
		SetFieldIf(FIELD_ID_GeoLocation_AltitudeRef, parser.FetchAs<uint8_t>(GeoLocation.AltitudeRef));
		break;

	case 6:
		// GPS altitude
		SetFieldIf(FIELD_ID_GeoLocation_Altitude, parser.Fetch(GeoLocation.Altitude));
		break;

	case 7:
		// GPS timestamp
		if (parser.IsRational() && parser.GetLength() == 3) {
			// Fetch() leaves its out-parameter untouched when the bounds check
			// rejects the offset, so the three are initialized and all three must
			// succeed: hour, minute and second are one logical value and a partial
			// timestamp is not a timestamp
			double h(0), m(0), s(0);
			if (parser.Fetch(h, 0) && parser.Fetch(m, 1) && parser.Fetch(s, 2)) {
				char buffer[256];
				snprintf(buffer, 256, "%g %g %g", h, m, s);
				GeoLocation.GPSTimeStamp = buffer;
				SetField(FIELD_ID_GeoLocation_GPSTimeStamp);
			}
		}
		break;

	case 11:
		// Indicates the GPS DOP (data degree of precision)
		SetFieldIf(FIELD_ID_GeoLocation_GPSDOP, parser.Fetch(GeoLocation.GPSDOP));
		break;

	case 18:
		// GPS geodetic survey data
		SetFieldIf(FIELD_ID_GeoLocation_GPSMapDatum, parser.Fetch(GeoLocation.GPSMapDatum));
		break;

	case 29:
		// GPS date-stamp
		SetFieldIf(FIELD_ID_GeoLocation_GPSDateStamp, parser.Fetch(GeoLocation.GPSDateStamp));
		break;

	case 30:
		// GPS differential indicates whether differential correction is applied to the GPS receiver
		SetFieldIf(FIELD_ID_GeoLocation_GPSDifferential, parser.Fetch(GeoLocation.GPSDifferential));
		break;
	}
}


//
// Scans the segments up to the image data for the metadata ones: the JM_APP1 EXIF and
// XMP segments, parsed by parseFromEXIFSegment() and parseFromXMPSegment(), and the
// JM_APP2 MPF index, parsed by parseFromMPFSegment()
//
int EXIFInfo::parseFrom(EXIFStream& input) {
	clear();
	if (!input.IsValid())
		return PARSE_INVALID_JPEG;

	// Counts the bytes consumed, which gives the offset of every segment in the stream:
	// the MPF index locates each image relative to its own segment
	class EXIFStreamCounter : public EXIFStream {
	public:
		explicit EXIFStreamCounter(EXIFStream& stream)
			: stream(stream), position(0) {}
		bool IsValid() const override {
			return stream.IsValid();
		}
		const uint8_t* GetBuffer(unsigned desiredLength) override {
			const uint8_t* const buf(stream.GetBuffer(desiredLength));
			if (buf != NULL)
				position += desiredLength;
			return buf;
		}
		bool SkipBuffer(unsigned desiredLength) override {
			if (!stream.SkipBuffer(desiredLength))
				return false;
			position += desiredLength;
			return true;
		}
		uint64_t GetPosition() const {
			return position;
		}
	private:
		EXIFStream& stream;
		uint64_t position;
	} stream(input);

	// Sanity check: all JPEG files start with 0xFFD8 and end with 0xFFD9
	// This check also ensures that the user has supplied a correct value for len.
	const uint8_t* buf(stream.GetBuffer(2));
	if (buf == NULL || buf[0] != JM_START || buf[1] != JM_SOI)
		return PARSE_INVALID_JPEG;

	// Scan the segments, reading the length that starts each one.
	// Exit once the EXIF, XMP and MPF sections were all parsed.
	struct APP1S {
		uint32_t& val;
		inline APP1S(uint32_t& v) : val(v) {}
		inline operator uint32_t () const { return val; }
		inline operator uint32_t& () { return val; }
		inline int operator () (int code=PARSE_ABSENT_DATA) const { return val&FIELD_ALL ? (int)PARSE_SUCCESS : code; }
	} app1s(Fields);
	while (!(app1s == FIELD_ALL && HasField(FIELD_ID_MPImages)) && (buf=stream.GetBuffer(2)) != NULL) {
		// find next marker;
		// in cases of markers appended after the compressed data,
		// optional JM_START fill bytes may precede the marker
		if (*buf++ != JM_START)
			break;
		uint8_t marker;
		while ((marker=buf[0]) == JM_START && (buf=stream.GetBuffer(1)) != NULL);
		// select marker
		switch (marker) {
		case 0x00:
		case 0x01:
		case JM_START:
		case JM_RST0:
		case JM_RST1:
		case JM_RST2:
		case JM_RST3:
		case JM_RST4:
		case JM_RST5:
		case JM_RST6:
		case JM_RST7:
		case JM_SOI:
			continue; // a marker without a segment
		case JM_SOS: // start of stream: and we're done
		case JM_EOI: // no data? not good
			return app1s();
		}
		uint16_t sectionLength;
		if ((buf=stream.GetBuffer(2)) == NULL ||
			(sectionLength=EntryParser::parse16(buf, false)) <= 2)
			return app1s(PARSE_INVALID_JPEG);
		sectionLength -= 2; // the length counts its own 2 bytes
		switch (marker) {
		case JM_APP1:
			// EXIF or XMP; once both were found, the scan only goes on for MPF
			// and any further EXIF or XMP is skipped
			if (app1s == FIELD_ALL)
				break;
			if ((buf=stream.GetBuffer(sectionLength)) == NULL)
				return app1s(PARSE_INVALID_JPEG);
			switch (int ret=parseFromEXIFSegment(buf, sectionLength)) {
			case PARSE_ABSENT_DATA:
#ifndef TINYEXIF_NO_XMP_SUPPORT
				switch (ret=parseFromXMPSegment(buf, sectionLength)) {
				case PARSE_ABSENT_DATA:
					break;
				case PARSE_SUCCESS:
					app1s |= FIELD_XMP;
					break;
				default:
					return app1s(ret); // some error
				}
#endif // TINYEXIF_NO_XMP_SUPPORT
				break;
			case PARSE_SUCCESS:
				app1s |= FIELD_EXIF;
				break;
			default:
				return app1s(ret); // some error
			}
			continue;
		case JM_APP2:
			// MPF; the first index found is used, and the other APP2 segments,
			// e.g. ICC profiles, are skipped without being read
			if (HasField(FIELD_ID_MPImages) || sectionLength <= 4)
				break;
			if ((buf=stream.GetBuffer(4)) == NULL)
				return app1s(PARSE_INVALID_JPEG);
			sectionLength -= 4;
			if (!std::equal(buf, buf+4, "MPF"))
				break;
			if ((buf=stream.GetBuffer(sectionLength)) == NULL)
				return app1s(PARSE_INVALID_JPEG);
			// the TIFF header starts the bytes just read; a malformed index only
			// leaves MPImages empty, as the image and its metadata are still valid
			parseFromMPFSegment(buf, sectionLength, stream.GetPosition() - sectionLength);
			continue;
		}
		// skip the section
		if (!stream.SkipBuffer(sectionLength))
			return app1s(PARSE_INVALID_JPEG);
	}
	return app1s();
}


int EXIFInfo::parseFrom(std::istream& stream) {
	class EXIFStdStream : public EXIFStream {
	public:
		EXIFStdStream(std::istream& stream)
			: stream(stream) {
			// Would be nice to assert here that the stream was opened in binary mode, but
			// apparently that's not possible: https://stackoverflow.com/a/224259/19254
		}
		bool IsValid() const override {
			return !!stream;
		}
		const uint8_t* GetBuffer(unsigned desiredLength) override {
			buffer.resize(desiredLength);
			if (!stream.read(reinterpret_cast<char*>(buffer.data()), desiredLength))
				return NULL;
			return buffer.data();
		}
		bool SkipBuffer(unsigned desiredLength) override {
			return (bool)stream.seekg(desiredLength, std::ios::cur);
		}
	private:
		std::istream& stream;
		std::vector<uint8_t> buffer;
	};
	EXIFStdStream streamWrapper(stream);
	return parseFrom(streamWrapper);
}


int EXIFInfo::parseFrom(const uint8_t* buf, unsigned len) {
	class EXIFStreamBuffer : public EXIFStream {
	public:
		explicit EXIFStreamBuffer(const uint8_t* buf, unsigned len)
			: it(buf), end(buf+len) {}
		bool IsValid() const override {
			return it != NULL;
		}
		const uint8_t* GetBuffer(unsigned desiredLength) override {
			// compare lengths, not pointers: it+desiredLength may lie past the end of the
			// buffer, which is undefined behavior; a read may end exactly at the end
			if (desiredLength > (size_t)(end - it))
				return NULL;
			const uint8_t* const begin(it);
			it += desiredLength;
			return begin;
		}
		bool SkipBuffer(unsigned desiredLength) override {
			return GetBuffer(desiredLength) != NULL;
		}
	private:
		const uint8_t* it, * const end;
	};
	EXIFStreamBuffer stream(buf, len);
	return parseFrom(stream);
}

//
// Parse the TIFF header that starts at 'start' in 'buf', as found in the EXIF and the
// MPF segments; on success, return PARSE_SUCCESS (0) with the byte order the header
// states and the buffer offset of the first IFD. The header is 8 bytes:
//  2 bytes: 'II' or 'MM' for Intel or Motorola byte alignment
//  2 bytes: 0x002a
//  4 bytes: offset of the first IFD, relative to the header start
//
static int ParseTIFFHeader(const uint8_t* buf, unsigned len, unsigned start, bool& alignIntel, unsigned& first_ifd) {
	if ((uint64_t)start + 8 > len)
		return PARSE_CORRUPT_DATA;
	// The marker states the byte order of the file, never of the host: parse16() and
	// parse32() assemble every value from its bytes, so they already give the same
	// result on any CPU, and mixing in the host's byte order inverts them (#29)
	if (buf[start] == 'I' && buf[start+1] == 'I')
		alignIntel = true; // 1: Intel byte alignment (little-endian)
	else
	if (buf[start] == 'M' && buf[start+1] == 'M')
		alignIntel = false; // 0: Motorola byte alignment (big-endian)
	else
		return PARSE_UNKNOWN_BYTEALIGN;
	if (0x2a != EntryParser::parse16(buf + start + 2, alignIntel))
		return PARSE_CORRUPT_DATA;
	// the first IFD offset is relative to the TIFF header start and it is stored in the
	// 4 bytes it has to skip, so anything below 4 points back into the TIFF header
	const uint32_t first_ifd_offset = EntryParser::parse32(buf + start + 4, alignIntel);
	if (first_ifd_offset < 4)
		return PARSE_CORRUPT_DATA;
	const uint64_t ifd = (uint64_t)start + first_ifd_offset;
	if (ifd >= len)
		return PARSE_CORRUPT_DATA;
	first_ifd = (unsigned)ifd;
	return PARSE_SUCCESS;
}

//
// Main parsing function for an EXIF segment.
// Do a sanity check by looking for bytes "Exif\0\0".
// The marker has to contain at least the TIFF header, otherwise the
// JM_APP1 data is corrupt. So the minimum length specified here has to be:
//   6 bytes: "Exif\0\0" string
//   2 bytes: TIFF header (either "II" or "MM" string)
//   2 bytes: TIFF magic (short 0x2a00 in Motorola byte order)
//   4 bytes: Offset to first IFD
// =========
//  14 bytes
//
// PARAM: 'buf' start of the EXIF TIFF, which must be the bytes "Exif\0\0".
// PARAM: 'len' length of buffer
//
int EXIFInfo::parseFromEXIFSegment(const uint8_t* buf, unsigned len) {
	unsigned offs = 6; // current offset into buffer
	if (!buf || len < offs)
		return PARSE_ABSENT_DATA;
	if (!std::equal(buf, buf+offs, "Exif\0\0"))
		return PARSE_ABSENT_DATA;

	const unsigned tiff_header_start = offs;
	bool alignIntel;
	if (const int ret = ParseTIFFHeader(buf, len, tiff_header_start, alignIntel, offs))
		return ret;
	EntryParser parser(buf, len, tiff_header_start, alignIntel);

	// Now parsing the first Image File Directory (IFD0, for the main image).
	// An IFD consists of a variable number of 12-byte directory entries. The
	// first two bytes of the IFD section contain the number of directory
	// entries in the section. The last 4 bytes of the IFD contain an offset
	// to the next IFD, which means this IFD must contain exactly 6 + 12 * num
	// bytes of data.
	// Note that it's possible that the next IFD offset doesn't exist,
	// so here the last 4 bytes are considered optional.
	if (!parser.InBounds(offs, 2))
		return PARSE_CORRUPT_DATA;
	unsigned num_entries = EntryParser::parse16(buf + offs, alignIntel);
	if (!parser.InBounds((uint64_t)offs + 2, 12 * num_entries))
		return PARSE_CORRUPT_DATA;
	uint64_t exif_sub_ifd_offset = len;
	uint64_t gps_sub_ifd_offset  = len;
	parser.Init(offs+2);
	while (num_entries-- > 0) {
		if (!parser.ParseTag())
			break;
		parseIFDImage(parser, exif_sub_ifd_offset, gps_sub_ifd_offset);
	}

	// Jump to the EXIF SubIFD if it exists and parse all the information
	// there. Note that it's possible that the EXIF SubIFD doesn't exist.
	// The EXIF SubIFD contains most of the interesting information that a
	// typical user might want.
	if (parser.InBounds(exif_sub_ifd_offset, 4)) {
		offs = (unsigned)exif_sub_ifd_offset;
		num_entries = EntryParser::parse16(buf + offs, alignIntel);
		if (!parser.InBounds((uint64_t)offs + 2, 12 * num_entries))
			return PARSE_CORRUPT_DATA;
		parser.Init(offs+2);
		while (num_entries-- > 0) {
			if (!parser.ParseTag())
				break;
			parseIFDExif(parser);
		}
	}

	// Jump to the GPS SubIFD if it exists and parse all the information
	// there. Note that it's possible that the GPS SubIFD doesn't exist.
	if (parser.InBounds(gps_sub_ifd_offset, 4)) {
		offs = (unsigned)gps_sub_ifd_offset;
		num_entries = EntryParser::parse16(buf + offs, alignIntel);
		if (!parser.InBounds((uint64_t)offs + 2, 12 * num_entries))
			return PARSE_CORRUPT_DATA;
		// GPSStatus follows the position tags, so it is looked up first
		const bool positionVoid(IsGPSMeasurementVoid(parser, offs+2, num_entries));
		parser.Init(offs+2);
		while (num_entries-- > 0) {
			if (!parser.ParseTag())
				break;
			if (!positionVoid || !IsGPSPositionTag(parser.GetTag()))
				parseIFDGPS(parser);
		}
		GeoLocation.parseCoords();
	}

	return PARSE_SUCCESS;
}

//
// Parsing function for the index of a Multi-Picture Format (MPF, CIPA DC-007) file:
// the APP2 segment "MPF\0", followed by a TIFF header of its own, with its own byte
// order, and the MP Index IFD. Its MP Entry tag holds one 16-byte entry per image
// stored in the file, this one first:
//  4 bytes: attribute: flags (bits 27-31), image format (bits 24-26), MP type (bits 0-23)
//  4 bytes: size of the image
//  4 bytes: offset of the image, relative to the TIFF header; 0 for the first image
//  4 bytes: entry numbers of two dependent images (not parsed)
// The images are only listed, not read: they lie after this image, outside the buffer.
//
// PARAM: 'buf' start of the TIFF header, which follows the bytes "MPF\0".
// PARAM: 'len' length of buffer
// PARAM: 'offset' offset of the TIFF header from the start of the JPEG stream
//
int EXIFInfo::parseFromMPFSegment(const uint8_t* buf, unsigned len, uint64_t offset) {
	bool alignIntel;
	unsigned offs;
	if (const int ret = ParseTIFFHeader(buf, len, 0, alignIntel, offs))
		return ret;
	EntryParser parser(buf, len, 0, alignIntel);
	if (!parser.InBounds(offs, 2))
		return PARSE_CORRUPT_DATA;
	unsigned num_entries = EntryParser::parse16(buf + offs, alignIntel);
	if (!parser.InBounds((uint64_t)offs + 2, 12 * num_entries))
		return PARSE_CORRUPT_DATA;
	parser.Init(offs+2);
	while (num_entries-- > 0 && parser.ParseTag()) {
		if (parser.GetTag() != 0xb002)
			continue;
		// MP Entry: the count is in bytes, so it bounds the entries by the segment size,
		// at most 4095 of them, before anything is allocated
		const uint32_t num_images(parser.GetLength() / 16);
		const uint64_t entries(parser.GetSubIFD());
		if (!parser.IsUndefined() || num_images == 0 || parser.GetLength() % 16 != 0 ||
			!parser.InBounds(entries, parser.GetLength()))
			return PARSE_CORRUPT_DATA;
		MPImages.resize(num_images);
		for (uint32_t i=0; i<num_images; ++i) {
			const uint8_t* const entry(buf + (size_t)entries + i * 16);
			const uint32_t attribute(EntryParser::parse32(entry, alignIntel));
			const uint32_t start(EntryParser::parse32(entry + 8, alignIntel));
			MPImage_t& image(MPImages[i]);
			image.Type   = attribute & 0x00ffffff;
			image.Format = (attribute >> 24) & 0x07;
			image.Flags  = (uint8_t)(attribute >> 27);
			image.Length = EntryParser::parse32(entry + 4, alignIntel);
			// 64bit, as the 32bit start counts from the header, not from the stream start
			image.Offset = start == 0 ? 0 : offset + start;
		}
		SetField(FIELD_ID_MPImages);
		return PARSE_SUCCESS;
	}
	return PARSE_ABSENT_DATA;
}

#ifndef TINYEXIF_NO_XMP_SUPPORT

//
// Main parsing function for a XMP segment.
// Do a sanity check by looking for bytes "http://ns.adobe.com/xap/1.0/\0".
// So the minimum length specified here has to be:
//  29 bytes: "http://ns.adobe.com/xap/1.0/\0" string
//
// PARAM: 'buf' start of the XMP header, which must be the bytes "http://ns.adobe.com/xap/1.0/\0".
// PARAM: 'len' length of buffer
//
int EXIFInfo::parseFromXMPSegment(const uint8_t* buf, unsigned len) {
	unsigned offs = 29; // current offset into buffer
	if (!buf || len < offs)
		return PARSE_ABSENT_DATA;
	if (!std::equal(buf, buf+offs, "http://ns.adobe.com/xap/1.0/\0"))
		return PARSE_ABSENT_DATA;
	if (offs >= len)
		return PARSE_CORRUPT_DATA;
	return parseFromXMPSegmentXML((const char*)(buf + offs), len - offs);
}
int EXIFInfo::parseFromXMPSegmentXML(const char* szXML, unsigned len) {
	// Skip xpacket end section so that tinyxml2 lib parses the section correctly.
	const char* szEnd(Tools::strrnstr(szXML, "<?xpacket end=", len));
	if (szEnd != NULL)
		len = (unsigned)(szEnd - szXML);

	// Try parsing the XML packet.
	tinyxml2::XMLDocument doc;
	const tinyxml2::XMLElement* document;
	if (doc.Parse(szXML, len) != tinyxml2::XML_SUCCESS ||
		((document=doc.FirstChildElement("x:xmpmeta")) == NULL && (document=doc.FirstChildElement("xmp:xmpmeta")) == NULL) ||
		(document=document->FirstChildElement("rdf:RDF")) == NULL ||
		(document=document->FirstChildElement("rdf:Description")) == NULL)
		return PARSE_ABSENT_DATA;

	struct ParseXMP	{
		// try yo fetch the value both from the attribute and child element
		// and parse if needed rational numbers stored as string fraction;
		// only a finite result is stored: a zero denominator divides to inf or NaN
		static bool Value(const tinyxml2::XMLElement* document, const char* name, double& value) {
			const char* szAttribute = document->Attribute(name);
			if (szAttribute == NULL) {
				const tinyxml2::XMLElement* const element(document->FirstChildElement(name));
				if (element == NULL || (szAttribute=element->GetText()) == NULL)
					return false;
			}
			std::vector<std::string> values;
			Tools::strSplit(szAttribute, '/', values);
			double numerator(0), denominator(0);
			switch (values.size()) {
			case 1: return Tools::strToDouble(values.front().c_str(), value);
			case 2: return Tools::strToDouble(values.front().c_str(), numerator) &&
				Tools::strToDouble(values.back().c_str(), denominator) &&
				AssignFinite(value, numerator/denominator);
			}
			return false;
		}
		// same as previous function but with unsigned int results;
		// values too large for uint32_t (a video item longer than 4GiB, for example) saturate
		// at UINT32_MAX instead of silently wrapping, and text that is not one whole number,
		// with no digits at all or with other text after them, is reported as absent instead
		// of as a zero or as the digits alone; strtoull, not strtoul, is used so the range
		// check is meaningful where unsigned long is only 32 bits wide.
		// note UINT32_MAX doubles as the "absent" sentinel of seven fields fed from here -
		// Distortion.DewarpFlag and the six GPano pixel counts - so for those a saturated
		// value reads back as absent through their hasXxx(); fail-safe, but not distinguishable.
		//
		// The double overload above has the same collision with its own sentinel: strtod
		// parses "1.7976931348623157e308" to exactly DBL_MAX, which clear() uses to mean
		// absent, so an XMP file carrying that literal sets the field and still reads back
		// as absent through hasAltitude(), hasRelativeAltitude(), hasOrientation() (Roll,
		// Pitch and Yaw), hasPosePitchDegrees(), hasPoseRollDegrees() and
		// hasPoseHeadingDegrees() - every DBL_MAX-sentinelled accessor this path can feed;
		// hasLatLon() and hasSpeed() also use DBL_MAX but are fed from EXIF rationals and
		// floats, neither of which can produce it. HasField()/GetFields() report it present,
		// so the two presence APIs disagree on this one input. The sentinels are part of
		// the public contract of those accessors and cannot be changed without breaking
		// every existing caller, so this is documented rather than fixed; HasField() is
		// the accurate answer where the two differ. Both collisions need a value at the
		// very edge of the type to trigger and both fail towards "absent".
		static bool Value(const tinyxml2::XMLElement* document, const char* name, uint32_t& value) {
			const char* szAttribute = document->Attribute(name);
			if (szAttribute == NULL) {
				const tinyxml2::XMLElement* const element(document->FirstChildElement(name));
				if (element == NULL || (szAttribute = element->GetText()) == NULL)
					return false;
			}
			// strtoull negates a negative input rather than rejecting it, so "-1" would
			// come back as ULLONG_MAX and saturate onto the UINT32_MAX absence sentinel;
			// a pixel count or a flag is never negative, so reject the sign outright
			const char* szValue(szAttribute);
			while (isspace((unsigned char)*szValue))
				++szValue;
			if (*szValue == '-')
				return false;
			char* szEnd(NULL);
			errno = 0;
			// base 10, not 0: these are decimal XMP integers, not C literals, so neither
			// a 0x prefix nor a leading zero should change how they are read
			const unsigned long long ullValue(strtoull(szValue, &szEnd, 10));
			if (!Tools::isWholeNumber(szValue, szEnd))
				return false;
			value = (errno == ERANGE || ullValue > UINT32_MAX ? UINT32_MAX : (uint32_t)ullValue);
			return true;
		}
		// same as previous function but with std::string
		static bool Value(const tinyxml2::XMLElement* document, const char* name, std::string& value) {
			const char* szAttribute = document->Attribute(name);
			if (szAttribute == NULL) {
				const tinyxml2::XMLElement* const element(document->FirstChildElement(name));
				if (element == NULL || (szAttribute = element->GetText()) == NULL)
					return false;
			}
			value = std::string(szAttribute);
			return true;
		}
		// true if the given mime type names a video; mime types are case insensitive
		static bool IsVideoMime(const std::string& mime) {
			std::string lower(mime);
			for (std::string::iterator it=lower.begin(); it!=lower.end(); ++it)
				*it = (char)tolower((unsigned char)*it);
			return lower.compare(0, 6, "video/") == 0;
		}
		// fetch the mime type and the byte length of the first video item listed in the
		// GCamera:MotionPhoto container directory:
		//  Container:Directory / rdf:Seq / rdf:li / Container:Item[Item:Mime, Item:Length]
		// some writers spell the container namespace "GContainer" and fold the item
		// namespace into the attribute name, so both spellings are tried;
		// this is XMP from an untrusted file, so every step of the walk may be missing;
		// 'hasLength' reports whether the item carried a length, which the caller needs
		// as this being a static of a local struct keeps it from marking the field itself
		static bool VideoItem(const tinyxml2::XMLElement* document, std::string& mime, uint32_t& length, bool& hasLength) {
			const char* const szDirectories[2] = {"Container:Directory", "GContainer:Directory"};
			const char* const szItems[2] = {"Container:Item", "GContainer:Item"};
			for (unsigned i=0; i<2; ++i) {
				const tinyxml2::XMLElement* const directory(document->FirstChildElement(szDirectories[i]));
				if (directory == NULL)
					continue;
				const tinyxml2::XMLElement* const seq(directory->FirstChildElement("rdf:Seq"));
				if (seq == NULL)
					continue;
				for (const tinyxml2::XMLElement* li(seq->FirstChildElement("rdf:li")); li != NULL; li=li->NextSiblingElement("rdf:li")) {
					const tinyxml2::XMLElement* const item(li->FirstChildElement(szItems[i]));
					if (item == NULL)
						continue;
					std::string itemMime;
					if (!Value(item, "Item:Mime", itemMime) &&
						!Value(item, "GContainer:ItemMime", itemMime))
						continue;
					if (!IsVideoMime(itemMime))
						continue;
					mime = itemMime;
					// a container item may legitimately omit its length
					hasLength =
						Value(item, "Item:Length", length) ||
						Value(item, "GContainer:ItemLength", length);
					return true;
				}
			}
			return false;
		}
	};

	// Try parsing the XMP content for tiff details.
	// these fill the same fields as their EXIF counterparts, so either source
	// finding one counts as present; they are all read by ParseXMP, like every other
	// XMP number: tinyxml2's QueryDoubleAttribute would cut the XMP rationals of the
	// resolutions ("300/1") at the slash, and its QueryUnsignedAttribute accepts text
	// after the digits and wraps a negative value around; a value too large for the
	// 16bit fields is out of range rather than truncated
	uint32_t orientation(0), resolutionUnit(0);
	if (Orientation == 0 &&
		SetFieldIf(FIELD_ID_Orientation, ParseXMP::Value(document, "tiff:Orientation", orientation) && orientation <= UINT16_MAX))
		Orientation = (uint16_t)orientation;
	if (ImageWidth == 0 && ImageHeight == 0) {
		SetFieldIf(FIELD_ID_ImageWidth, ParseXMP::Value(document, "tiff:ImageWidth", ImageWidth));
		if (!SetFieldIf(FIELD_ID_ImageHeight, ParseXMP::Value(document, "tiff:ImageHeight", ImageHeight)))
			SetFieldIf(FIELD_ID_ImageHeight, ParseXMP::Value(document, "tiff:ImageLength", ImageHeight));
	}
	if (XResolution == 0 && YResolution == 0 && ResolutionUnit == 0) {
		SetFieldIf(FIELD_ID_XResolution, ParseXMP::Value(document, "tiff:XResolution", XResolution));
		SetFieldIf(FIELD_ID_YResolution, ParseXMP::Value(document, "tiff:YResolution", YResolution));
		if (SetFieldIf(FIELD_ID_ResolutionUnit, ParseXMP::Value(document, "tiff:ResolutionUnit", resolutionUnit) && resolutionUnit <= UINT16_MAX))
			ResolutionUnit = (uint16_t)resolutionUnit;
	}

	// Try parsing the XMP content for supported maker's info.
	const char* szAbout(document->Attribute("rdf:about"));
	if (0 == strcasecmp(Make.c_str(), "DJI") || (szAbout != NULL && 0 == strcasecmp(szAbout, "DJI Meta Data"))) {
		// the absolute altitude needs a GPS fix, and without one DJI marks it invalid
		// through GpsStatus, as EXIF does through GPSStatus 'V'
		std::string gpsStatus;
		if (!ParseXMP::Value(document, "drone-dji:GpsStatus", gpsStatus) || 0 != strcasecmp(gpsStatus.c_str(), "Invalid"))
			SetFieldIf(FIELD_ID_GeoLocation_Altitude, ParseXMP::Value(document, "drone-dji:AbsoluteAltitude", GeoLocation.Altitude));
		SetFieldIf(FIELD_ID_GeoLocation_RelativeAltitude, ParseXMP::Value(document, "drone-dji:RelativeAltitude", GeoLocation.RelativeAltitude));
		SetFieldIf(FIELD_ID_GeoLocation_RollDegree, ParseXMP::Value(document, "drone-dji:GimbalRollDegree", GeoLocation.RollDegree));
		SetFieldIf(FIELD_ID_GeoLocation_PitchDegree, ParseXMP::Value(document, "drone-dji:GimbalPitchDegree", GeoLocation.PitchDegree));
		SetFieldIf(FIELD_ID_GeoLocation_YawDegree, ParseXMP::Value(document, "drone-dji:GimbalYawDegree", GeoLocation.YawDegree));
		SetFieldIf(FIELD_ID_Calibration_FocalLength, ParseXMP::Value(document, "drone-dji:CalibratedFocalLength", Calibration.FocalLength));
		SetFieldIf(FIELD_ID_Calibration_OpticalCenterX, ParseXMP::Value(document, "drone-dji:CalibratedOpticalCenterX", Calibration.OpticalCenterX));
		SetFieldIf(FIELD_ID_Calibration_OpticalCenterY, ParseXMP::Value(document, "drone-dji:CalibratedOpticalCenterY", Calibration.OpticalCenterY));
		// flight speed: the DJI MakerNote has it at binary precision and wins in either
		// segment order, as it overwrites while this XMP text only fills in a missing one
		if (!HasField(FIELD_ID_GeoLocation_SpeedX))
			SetFieldIf(FIELD_ID_GeoLocation_SpeedX, ParseXMP::Value(document, "drone-dji:FlightXSpeed", GeoLocation.SpeedX));
		if (!HasField(FIELD_ID_GeoLocation_SpeedY))
			SetFieldIf(FIELD_ID_GeoLocation_SpeedY, ParseXMP::Value(document, "drone-dji:FlightYSpeed", GeoLocation.SpeedY));
		if (!HasField(FIELD_ID_GeoLocation_SpeedZ))
			SetFieldIf(FIELD_ID_GeoLocation_SpeedZ, ParseXMP::Value(document, "drone-dji:FlightZSpeed", GeoLocation.SpeedZ));
		std::string dewarpData;
		SetFieldIf(FIELD_ID_Distortion_DewarpFlag, ParseXMP::Value(document, "drone-dji:DewarpFlag", Distortion.DewarpFlag));
		// DewarpData lands in a local: the fields it feeds are marked below, where they are written
		ParseXMP::Value(document, "drone-dji:DewarpData", dewarpData);
		std::vector<double> distortionParams;
		size_t pos = dewarpData.find(';');
		if (pos != std::string::npos) {
			std::stringstream ss(dewarpData.substr(pos + 1));
			std::string item;
			while (std::getline(ss, item, ',')) {
				// strtod, not std::stod: dewarpData is attacker controlled XMP text and
				// std::stod throws on a non-numeric or an out-of-range item, an exception
				// nothing between here and parseFrom() catches - it would abort the
				// process instead of returning one of the documented error codes
				double value(0);
				if (!Tools::strToDouble(item.c_str(), value)) {
					// one malformed item invalidates the whole list, so that the
					// distortion fields stay absent instead of half populated
					distortionParams.clear();
					break;
				}
				distortionParams.push_back(value);
			}
		}
		// The DewarpData string has the following format:
		// date;Fx,Fy,Cx,Cy,K1,K2,P1,P2,K3
		// , where Fx, Fy are focal lengths in pixels, Cx, Cy are optical center offsets from the image center in pixels
		if (distortionParams.size() == 9) {
			Distortion.K1 = distortionParams[4];
			Distortion.K2 = distortionParams[5];
			Distortion.P1 = distortionParams[6];
			Distortion.P2 = distortionParams[7];
			Distortion.K3 = distortionParams[8];
			SetField(FIELD_ID_Distortion_K1);
			SetField(FIELD_ID_Distortion_K2);
			SetField(FIELD_ID_Distortion_P1);
			SetField(FIELD_ID_Distortion_P2);
			SetField(FIELD_ID_Distortion_K3);
		}
	} else
	if (0 == strcasecmp(Make.c_str(), "senseFly") || 0 == strcasecmp(Make.c_str(), "Sentera")) {
		SetFieldIf(FIELD_ID_GeoLocation_RollDegree, ParseXMP::Value(document, "Camera:Roll", GeoLocation.RollDegree));
		if (SetFieldIf(FIELD_ID_GeoLocation_PitchDegree, ParseXMP::Value(document, "Camera:Pitch", GeoLocation.PitchDegree))) {
			// convert to DJI format: senseFly uses pitch 0 as NADIR, whereas DJI -90
			GeoLocation.PitchDegree = Tools::NormD180(GeoLocation.PitchDegree-90.0);
		}
		SetFieldIf(FIELD_ID_GeoLocation_YawDegree, ParseXMP::Value(document, "Camera:Yaw", GeoLocation.YawDegree));
		SetFieldIf(FIELD_ID_GeoLocation_AccuracyXY, ParseXMP::Value(document, "Camera:GPSXYAccuracy", GeoLocation.AccuracyXY));
		SetFieldIf(FIELD_ID_GeoLocation_AccuracyZ, ParseXMP::Value(document, "Camera:GPSZAccuracy", GeoLocation.AccuracyZ));
	} else
	if (0 == strcasecmp(Make.c_str(), "PARROT")) {
		SetFieldIf(FIELD_ID_GeoLocation_RollDegree, ParseXMP::Value(document, "Camera:Roll", GeoLocation.RollDegree)) ||
		SetFieldIf(FIELD_ID_GeoLocation_RollDegree, ParseXMP::Value(document, "drone-parrot:CameraRollDegree", GeoLocation.RollDegree));
		if (SetFieldIf(FIELD_ID_GeoLocation_PitchDegree, ParseXMP::Value(document, "Camera:Pitch", GeoLocation.PitchDegree)) ||
			SetFieldIf(FIELD_ID_GeoLocation_PitchDegree, ParseXMP::Value(document, "drone-parrot:CameraPitchDegree", GeoLocation.PitchDegree))) {
			// convert to DJI format: senseFly uses pitch 0 as NADIR, whereas DJI -90
			GeoLocation.PitchDegree = Tools::NormD180(GeoLocation.PitchDegree-90.0);
		}
		SetFieldIf(FIELD_ID_GeoLocation_YawDegree, ParseXMP::Value(document, "Camera:Yaw", GeoLocation.YawDegree)) ||
		SetFieldIf(FIELD_ID_GeoLocation_YawDegree, ParseXMP::Value(document, "drone-parrot:CameraYawDegree", GeoLocation.YawDegree));
		SetFieldIf(FIELD_ID_GeoLocation_RelativeAltitude, ParseXMP::Value(document, "Camera:AboveGroundAltitude", GeoLocation.RelativeAltitude));
	}
	// Try parsing the XMP content for spherical (GPano) metadata.
	// GPano:ProjectionType is parsed once, into the raw spec string; the existing numeric
	// ProjectionType is derived from it below instead of being parsed independently, so the
	// two fields can never drift out of sync with each other.
	if (SetFieldIf(FIELD_ID_GPano_ProjectionType, ParseXMP::Value(document, "GPano:ProjectionType", GPano.ProjectionType))) {
		if (0 == strcasecmp(GPano.ProjectionType.c_str(), "perspective")) {
			ProjectionType = 1;
			SetField(FIELD_ID_ProjectionType);
		} else
		if (GPano.isEquirectangular()) {
			ProjectionType = 2;
			SetField(FIELD_ID_ProjectionType);
		}
	}
	SetFieldIf(FIELD_ID_GPano_PoseHeadingDegrees, ParseXMP::Value(document, "GPano:PoseHeadingDegrees", GPano.PoseHeadingDegrees));
	SetFieldIf(FIELD_ID_GPano_PosePitchDegrees, ParseXMP::Value(document, "GPano:PosePitchDegrees", GPano.PosePitchDegrees));
	SetFieldIf(FIELD_ID_GPano_PoseRollDegrees, ParseXMP::Value(document, "GPano:PoseRollDegrees", GPano.PoseRollDegrees));
	SetFieldIf(FIELD_ID_GPano_CroppedAreaImageWidthPixels, ParseXMP::Value(document, "GPano:CroppedAreaImageWidthPixels", GPano.CroppedAreaImageWidthPixels));
	SetFieldIf(FIELD_ID_GPano_CroppedAreaImageHeightPixels, ParseXMP::Value(document, "GPano:CroppedAreaImageHeightPixels", GPano.CroppedAreaImageHeightPixels));
	SetFieldIf(FIELD_ID_GPano_FullPanoWidthPixels, ParseXMP::Value(document, "GPano:FullPanoWidthPixels", GPano.FullPanoWidthPixels));
	SetFieldIf(FIELD_ID_GPano_FullPanoHeightPixels, ParseXMP::Value(document, "GPano:FullPanoHeightPixels", GPano.FullPanoHeightPixels));
	SetFieldIf(FIELD_ID_GPano_CroppedAreaLeftPixels, ParseXMP::Value(document, "GPano:CroppedAreaLeftPixels", GPano.CroppedAreaLeftPixels));
	SetFieldIf(FIELD_ID_GPano_CroppedAreaTopPixels, ParseXMP::Value(document, "GPano:CroppedAreaTopPixels", GPano.CroppedAreaTopPixels));

	// parse GCamera:MicroVideo
	if (document->Attribute("GCamera:MicroVideo")) {
		SetFieldIf(FIELD_ID_MicroVideo_HasMicroVideo, ParseXMP::Value(document, "GCamera:MicroVideo", MicroVideo.HasMicroVideo));
		SetFieldIf(FIELD_ID_MicroVideo_MicroVideoVersion, ParseXMP::Value(document, "GCamera:MicroVideoVersion", MicroVideo.MicroVideoVersion));
		SetFieldIf(FIELD_ID_MicroVideo_MicroVideoOffset, ParseXMP::Value(document, "GCamera:MicroVideoOffset", MicroVideo.MicroVideoOffset));
	}
	// parse GCamera:MotionPhoto, the container format that supersedes GCamera:MicroVideo;
	// deliberately not an "else" of the block above: the two write to disjoint fields, so a
	// transitional file declaring both attributes reports both instead of losing one of them,
	// and neither can overwrite the other's data
	if (document->Attribute("GCamera:MotionPhoto")) {
		SetFieldIf(FIELD_ID_MicroVideo_HasMotionPhoto, ParseXMP::Value(document, "GCamera:MotionPhoto", MicroVideo.HasMotionPhoto));
		// the container gives the video item's *length*; it is deliberately not stored in
		// MicroVideoOffset, which is an offset from the end of the file - a different
		// quantity as soon as the container lists any item after the video.
		// it is only walked when the file actually claims a motion photo: one saying
		// GCamera:MotionPhoto="0" while carrying a container for something else (an Ultra HDR
		// gain map, say) must not come back with the payload fields filled in, or
		// HasMotionPhoto would no longer tell "no motion photo" from "length unknown"
		if (MicroVideo.HasMotionPhoto) {
			bool hasLength(false);
			if (SetFieldIf(FIELD_ID_MicroVideo_MotionPhotoMime, ParseXMP::VideoItem(document, MicroVideo.MotionPhotoMime, MicroVideo.MotionPhotoLength, hasLength)))
				SetFieldIf(FIELD_ID_MicroVideo_MotionPhotoLength, hasLength);
		}
	}
	return PARSE_SUCCESS;
}

#endif // TINYEXIF_NO_XMP_SUPPORT

bool EXIFInfo::Calibration_t::hasCalibration() const {
	return FocalLength > 0.0 && OpticalCenterX > 0.0 && OpticalCenterY > 0.0;
}

bool EXIFInfo::Distortion_t::hasDewarpFlag() const {
	return DewarpFlag != UINT32_MAX;
}
bool EXIFInfo::Distortion_t::hasDistortion() const {
	return K1 != 0.0 || K2 != 0.0 || P1 != 0.0 || P2 != 0.0 || K3 != 0.0;
}

// Negate a magnitude without turning 0 into IEEE 754 -0.0, which prints as "-0": a
// position on the equator, the prime meridian or at sea level is 0 whatever its ref
static double Negate(double magnitude) {
	return magnitude == 0 ? 0.0 : -magnitude;
}

void EXIFInfo::Geolocation_t::parseCoords() {
	// Convert GPS latitude
	if (LatComponents.degrees != DBL_MAX ||
		LatComponents.minutes != 0 ||
		LatComponents.seconds != 0) {
		Latitude =
			LatComponents.degrees +
			LatComponents.minutes / 60 +
			LatComponents.seconds / 3600;
		if ('S' == LatComponents.direction)
			Latitude = Negate(Latitude);
	}
	// Convert GPS longitude
	if (LonComponents.degrees != DBL_MAX ||
		LonComponents.minutes != 0 ||
		LonComponents.seconds != 0) {
		Longitude =
			LonComponents.degrees +
			LonComponents.minutes / 60 +
			LonComponents.seconds / 3600;
		if ('W' == LonComponents.direction)
			Longitude = Negate(Longitude);
	}
	// Convert GPS altitude
	if (hasAltitude() &&
		(AltitudeRef == 1 || AltitudeRef == 3)) {
		Altitude = Negate(std::abs(Altitude));
	}
}

bool EXIFInfo::Geolocation_t::hasLatLon() const {
	return Latitude != DBL_MAX && Longitude != DBL_MAX;
}
bool EXIFInfo::Geolocation_t::hasAltitude() const {
	return Altitude != DBL_MAX;
}
bool EXIFInfo::Geolocation_t::hasRelativeAltitude() const {
	return RelativeAltitude != DBL_MAX;
}
bool EXIFInfo::Geolocation_t::hasOrientation() const {
	return RollDegree != DBL_MAX && PitchDegree != DBL_MAX && YawDegree != DBL_MAX;
}
bool EXIFInfo::Geolocation_t::hasSpeed() const {
	return SpeedX != DBL_MAX && SpeedY != DBL_MAX && SpeedZ != DBL_MAX;
}
bool EXIFInfo::Geolocation_t::hasAccuracy() const {
	return AccuracyXY != 0 && AccuracyZ != 0;
}

bool EXIFInfo::GPano_t::hasPosePitchDegrees() const {
	return PosePitchDegrees != DBL_MAX;
}

bool EXIFInfo::GPano_t::hasPoseRollDegrees() const {
	return PoseRollDegrees != DBL_MAX;
}

bool EXIFInfo::GPano_t::hasPoseHeadingDegrees() const {
	return PoseHeadingDegrees != DBL_MAX;
}

bool EXIFInfo::GPano_t::hasCroppedAreaImageWidthPixels() const {
	return CroppedAreaImageWidthPixels != UINT32_MAX;
}

bool EXIFInfo::GPano_t::hasCroppedAreaImageHeightPixels() const {
	return CroppedAreaImageHeightPixels != UINT32_MAX;
}

bool EXIFInfo::GPano_t::hasFullPanoWidthPixels() const {
	return FullPanoWidthPixels != UINT32_MAX;
}

bool EXIFInfo::GPano_t::hasFullPanoHeightPixels() const {
	return FullPanoHeightPixels != UINT32_MAX;
}

bool EXIFInfo::GPano_t::hasCroppedAreaLeftPixels() const {
	return CroppedAreaLeftPixels != UINT32_MAX;
}

bool EXIFInfo::GPano_t::hasCroppedAreaTopPixels() const {
	return CroppedAreaTopPixels != UINT32_MAX;
}

bool EXIFInfo::GPano_t::isEquirectangular() const {
	return 0 == strcasecmp(ProjectionType.c_str(), "equirectangular") ||
		0 == strcasecmp(ProjectionType.c_str(), "spherical");
}

bool EXIFInfo::MPImage_t::isLargeThumbnail() const {
	return (Type & 0xff0000) == 0x010000;
}

void EXIFInfo::clear() {
	Fields = FIELD_NA;

	// Presence bits: nothing was found yet
	FieldsPresent.assign(FIELD_ID_WORDS, 0);

	// Strings
	ImageDescription  = "";
	Make              = "";
	Model             = "";
	SerialNumber      = "";
	Software          = "";
	DateTime          = "";
	DateTimeOriginal  = "";
	DateTimeDigitized = "";
	SubSecTimeOriginal= "";
	Copyright         = "";

	// Shorts / unsigned / double
	ImageWidth        = 0;
	ImageHeight       = 0;
	RelatedImageWidth = 0;
	RelatedImageHeight= 0;
	Orientation       = 0;
	XResolution       = 0;
	YResolution       = 0;
	ResolutionUnit    = 0;
	BitsPerSample     = 0;
	ExposureTime      = 0;
	FNumber           = 0;
	ExposureProgram   = 0;
	ISOSpeedRatings   = 0;
	ShutterSpeedValue = 0;
	ApertureValue     = 0;
	MaxApertureValue  = 0;
	BrightnessValue   = 0;
	ExposureBiasValue = 0;
	SubjectDistance   = 0;
	FocalLength       = 0;
	Flash             = 0;
	MeteringMode      = 0;
	LightSource       = 0;
	ProjectionType    = 0;
	SubjectArea.clear();

	// Calibration
	Calibration.FocalLength = 0;
	Calibration.OpticalCenterX = 0;
	Calibration.OpticalCenterY = 0;

	// LensInfo
	LensInfo.FocalLengthMax = 0;
	LensInfo.FocalLengthMin = 0;
	LensInfo.FStopMax = 0;
	LensInfo.FStopMin = 0;
	LensInfo.DigitalZoomRatio = 0;
	LensInfo.FocalLengthIn35mm = 0;
	LensInfo.FocalPlaneXResolution = 0;
	LensInfo.FocalPlaneYResolution = 0;
	LensInfo.FocalPlaneResolutionUnit = 0;
	LensInfo.Make = "";
	LensInfo.Model = "";

	// Geolocation
	GeoLocation.Latitude                = DBL_MAX;
	GeoLocation.Longitude               = DBL_MAX;
	GeoLocation.Altitude                = DBL_MAX;
	GeoLocation.AltitudeRef             = 0;
	GeoLocation.RelativeAltitude        = DBL_MAX;
	GeoLocation.RollDegree              = DBL_MAX;
	GeoLocation.PitchDegree             = DBL_MAX;
	GeoLocation.YawDegree               = DBL_MAX;
	GeoLocation.SpeedX                  = DBL_MAX;
	GeoLocation.SpeedY                  = DBL_MAX;
	GeoLocation.SpeedZ                  = DBL_MAX;
	GeoLocation.AccuracyXY              = 0;
	GeoLocation.AccuracyZ               = 0;
	GeoLocation.GPSDOP                  = 0;
	GeoLocation.GPSDifferential         = 0;
	GeoLocation.GPSMapDatum             = "";
	GeoLocation.GPSTimeStamp            = "";
	GeoLocation.GPSDateStamp            = "";
	GeoLocation.LatComponents.degrees   = DBL_MAX;
	GeoLocation.LatComponents.minutes   = 0;
	GeoLocation.LatComponents.seconds   = 0;
	GeoLocation.LatComponents.direction = 0;
	GeoLocation.LonComponents.degrees   = DBL_MAX;
	GeoLocation.LonComponents.minutes   = 0;
	GeoLocation.LonComponents.seconds   = 0;
	GeoLocation.LonComponents.direction = 0;

	// Distortion
	Distortion.DewarpFlag = UINT32_MAX;
	Distortion.K1 = 0;
	Distortion.K2 = 0;
	Distortion.P1 = 0;
	Distortion.P2 = 0;
	Distortion.K3 = 0;

	// GPano
	GPano.PosePitchDegrees = DBL_MAX;
	GPano.PoseRollDegrees = DBL_MAX;
	GPano.PoseHeadingDegrees = DBL_MAX;
	GPano.ProjectionType = "";
	GPano.CroppedAreaImageWidthPixels = UINT32_MAX;
	GPano.CroppedAreaImageHeightPixels = UINT32_MAX;
	GPano.FullPanoWidthPixels = UINT32_MAX;
	GPano.FullPanoHeightPixels = UINT32_MAX;
	GPano.CroppedAreaLeftPixels = UINT32_MAX;
	GPano.CroppedAreaTopPixels = UINT32_MAX;

	// Video metadata
	MicroVideo.HasMicroVideo = 0;
	MicroVideo.MicroVideoVersion = 0;
	MicroVideo.MicroVideoOffset = 0;
	MicroVideo.HasMotionPhoto = 0;
	MicroVideo.MotionPhotoLength = 0;
	MicroVideo.MotionPhotoMime = "";

	// Multi-Picture Format
	MPImages.clear();
}

} // namespace TinyEXIF
