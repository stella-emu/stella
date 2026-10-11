# TinyEXIF: Tiny ISO-compliant C++ EXIF and XMP parsing library for JPEG

## Introduction

TinyEXIF is a tiny, lightweight C++11 library for parsing the metadata inside JPEG files: EXIF,
XMP, and the Multi-Picture Format index. EXIF parsing needs no third party dependency; XMP
parsing needs the [TinyXML2](https://github.com/leethomason/tinyxml2) library. To use it, copy
the two source files into your project, or build it with CMake, and pass the JPEG data to the
`EXIFInfo` class. Besides the common camera metadata (make and model, resolution, timestamps,
exposure, lens and GPS), it reads drone, spherical panorama and motion photo metadata. It is easy
to extend with any missing EXIF or XMP field.

## Features

- **Inputs**: a `std::istream`, of which only the segments before the image data are read; a
  memory buffer; your own `EXIFStream` implementation; or a lone EXIF or XMP segment.
- **EXIF**: image, camera, exposure, lens and GPS tags from IFD0, the Exif IFD and the GPS IFD, in
  either byte order, on little- and big-endian CPUs alike.
- **XMP** (optional): the `tiff:` image fields; DJI drone metadata (altitude, gimbal angles, flight
  speed, camera calibration, lens distortion); senseFly, Sentera and Parrot camera angles; Google
  spherical panoramas (`GPano:`) and motion photos (`GCamera:`). XMP stored inside the EXIF segment
  is read too.
- **DJI MakerNote**: flight speed and camera angles, in whichever byte order the file stores them.
- **Multi-Picture Format (MPF)**: locates the images stored after the main one, such as the preview
  of a panorama, the second view of a stereo image, or an HDR gain map.
- **Presence API**: tells a tag that is absent apart from one that is legitimately zero.
- **Built for untrusted files**: every read is bounds checked, malformed values are left absent,
  every floating point field is finite, and the parser is fuzzed and tested under sanitizers.

## Usage example

```cpp
#include "TinyEXIF.h"
#include <iostream> // std::cout
#include <fstream>  // std::ifstream
#include <vector>   // std::vector

int main(int argc, const char** argv) {
	if (argc != 2) {
		std::cout << "Usage: TinyEXIF <image_file>" << std::endl;
		return -1;
	}

	// open a stream to read just the necessary parts of the image file
	std::ifstream istream(argv[1], std::ifstream::binary);

	// parse image EXIF and XMP metadata
	TinyEXIF::EXIFInfo imageEXIF(istream);
	if (imageEXIF.Fields)
		std::cout
			<< "Image Description " << imageEXIF.ImageDescription << "\n"
			<< "Image Resolution " << imageEXIF.ImageWidth << "x" << imageEXIF.ImageHeight << " pixels\n"
			<< "Camera Model " << imageEXIF.Make << " - " << imageEXIF.Model << "\n"
			<< "Focal Length " << imageEXIF.FocalLength << " mm" << std::endl;
	return 0;
}
```

See `main.cpp` for a demo that prints every field.

## Reading a JPEG

`EXIFInfo` parses from its constructor, or from `parseFrom()` on an existing object, which also
returns how it went:

```cpp
	// from a stream opened in binary mode: reading stops before the image data
	std::ifstream file(path, std::ios::binary);
	TinyEXIF::EXIFInfo fromFile(file);

	// from memory
	TinyEXIF::EXIFInfo fromMemory(data, length);

	// reusing an object; parseFrom() clears what it held before
	TinyEXIF::EXIFInfo info;
	if (info.parseFrom(data, length) == TinyEXIF::PARSE_SUCCESS)
		std::cout << "EXIF " << ((info.Fields & TinyEXIF::FIELD_EXIF) ? "found" : "missing") << "\n";
```

| Return code | Meaning |
| --- | --- |
| `PARSE_SUCCESS` | EXIF and/or XMP were found and parsed |
| `PARSE_INVALID_JPEG` | not a JPEG: no start-of-image marker, or a broken segment before any metadata |
| `PARSE_UNKNOWN_BYTEALIGN` | the EXIF byte-order marker is neither `II` nor `MM` |
| `PARSE_ABSENT_DATA` | the JPEG holds no EXIF and no XMP |
| `PARSE_CORRUPT_DATA` | an EXIF or XMP segment was found but is corrupt |

Once a segment was parsed, an error in a later one still returns `PARSE_SUCCESS` with what was
found, and a malformed MPF index only leaves `MPImages` empty. `Fields` tells which segments were
found: `FIELD_EXIF`, `FIELD_XMP`, or both (`FIELD_ALL`).

The scan reads every segment up to the image data: the APP1 EXIF and XMP segments and the APP2
MPF index. Once it has found an EXIF and an XMP segment, any further one is skipped. Where both
carry a value for the same field, the segment parsed last wins, with two exceptions that only fill
in a missing value: the XMP `tiff:` fields, and the DJI flight speed from XMP, which the binary
MakerNote overrides.

**Other sources.** To read from anything else, implement `EXIFStream`:

```cpp
class MyStream : public TinyEXIF::EXIFStream {
public:
	bool IsValid() const override;                             // can the stream be read at all?
	const uint8_t* GetBuffer(unsigned desiredLength) override; // the next desiredLength bytes, or NULL if fewer remain;
	                                                           // the pointer must stay valid until the next call
	bool SkipBuffer(unsigned desiredLength) override;          // skip desiredLength bytes; false if fewer remain
};
```

**Single segments.** If you already have the metadata without the JPEG around it, use
`parseFromEXIFSegment()` on a blob that starts with `"Exif\0\0"`, `parseFromXMPSegment()` on one
that starts with `"http://ns.adobe.com/xap/1.0/\0"`, or `parseFromXMPSegmentXML()` on the XML
packet alone. They add to what the object already holds, so call `clear()` first for a fresh
result.

## Supported metadata

Each field below is a member of `EXIFInfo`; nested ones belong to the struct named in the heading.
EXIF tags are given with their number in the IFD that holds them. An XMP column lists the
properties that fill the same field.

### Image and camera

| Field | EXIF | XMP |
| --- | --- | --- |
| `ImageWidth`, `ImageHeight` | PixelXDimension, PixelYDimension (0xA002, 0xA003) | `tiff:ImageWidth`, `tiff:ImageHeight` or `tiff:ImageLength` |
| `RelatedImageWidth`, `RelatedImageHeight` | RelatedImageWidth, RelatedImageLength (0x1001, 0x1002) | |
| `ImageDescription` | ImageDescription (0x010E) | |
| `Make`, `Model` | Make, Model (0x010F, 0x0110) | |
| `SerialNumber` | BodySerialNumber (0xA431) | |
| `Orientation` | Orientation (0x0112) | `tiff:Orientation` |
| `XResolution`, `YResolution`, `ResolutionUnit` | 0x011A, 0x011B, 0x0128 | `tiff:XResolution`, `tiff:YResolution`, `tiff:ResolutionUnit` |
| `BitsPerSample` | BitsPerSample (0x0102) | |
| `Software` | Software (0x0131) | |
| `DateTime` | DateTime (0x0132), when the file was last changed | |
| `DateTimeOriginal`, `DateTimeDigitized`, `SubSecTimeOriginal` | 0x9003, 0x9004, 0x9291 | |
| `Copyright` | Copyright (0x8298) | |

### Exposure

| Field | EXIF |
| --- | --- |
| `ExposureTime` | ExposureTime (0x829A), in seconds |
| `FNumber` | FNumber (0x829D) |
| `ExposureProgram` | ExposureProgram (0x8822) |
| `ISOSpeedRatings` | ISOSpeedRatings (0x8827), or ExposureIndex (0xA215) clamped to 65535 |
| `ShutterSpeedValue` | ShutterSpeedValue (0x9201), converted from APEX to an exposure time in seconds |
| `ApertureValue` | ApertureValue (0x9202), converted from APEX to an f-number |
| `MaxApertureValue` | MaxApertureValue (0x9205), the widest aperture of the lens, converted from APEX to an f-number |
| `BrightnessValue` | BrightnessValue (0x9203), in APEX |
| `ExposureBiasValue` | ExposureBiasValue (0x9204), in EV |
| `SubjectDistance` | SubjectDistance (0x9206), in meters |
| `MeteringMode`, `LightSource`, `Flash` | 0x9207, 0x9208, 0x9209 |
| `FocalLength` | FocalLength (0x920A), in millimeters |
| `SubjectArea` | SubjectArea (0x9214), 2 to 4 values |

### Lens (`LensInfo`)

| Field | EXIF |
| --- | --- |
| `FocalLengthMin`, `FocalLengthMax`, `FStopMin`, `FStopMax` | LensSpecification (0xA432): the focal length range, and the widest f-number at each end of it |
| `DigitalZoomRatio` | DigitalZoomRatio (0xA404) |
| `FocalLengthIn35mm` | FocalLengthIn35mmFilm (0xA405) |
| `FocalPlaneXResolution`, `FocalPlaneYResolution`, `FocalPlaneResolutionUnit` | 0xA20E, 0xA20F, 0xA210 |
| `Make`, `Model` | LensMake, LensModel (0xA433, 0xA434) |

### GPS and flight (`GeoLocation`)

| Field | EXIF | XMP |
| --- | --- | --- |
| `Latitude`, `Longitude` | GPS tags 1 to 4, as signed decimal degrees; `LatComponents` and `LonComponents` keep the degrees, minutes, seconds and hemisphere | |
| `Altitude`, `AltitudeRef` | GPS tags 5 and 6, in meters, negative below sea level | `drone-dji:AbsoluteAltitude` |
| `RelativeAltitude` | | `drone-dji:RelativeAltitude`, Parrot `Camera:AboveGroundAltitude` |
| `GPSTimeStamp` | GPS tag 7, as `"h m s"` in UTC | |
| `GPSDOP` | GPS tag 11 | |
| `GPSMapDatum` | GPS tag 18 | |
| `GPSDateStamp` | GPS tag 29, as `"YYYY:MM:DD"` | |
| `GPSDifferential` | GPS tag 30 | |
| `RollDegree`, `PitchDegree`, `YawDegree` | DJI MakerNote camera angles | `drone-dji:GimbalRollDegree`, `GimbalPitchDegree`, `GimbalYawDegree`; senseFly, Sentera and Parrot `Camera:Roll`, `Pitch`, `Yaw`, or Parrot `drone-parrot:CameraRollDegree`, `CameraPitchDegree`, `CameraYawDegree` |
| `SpeedX`, `SpeedY`, `SpeedZ` | DJI MakerNote flight speed, in m/s | `drone-dji:FlightXSpeed`, `FlightYSpeed`, `FlightZSpeed` |
| `AccuracyXY`, `AccuracyZ` | | senseFly and Sentera `Camera:GPSXYAccuracy`, `Camera:GPSZAccuracy` |

A GPS receiver without a fix still writes the position tags, usually all zeros. When `GPSStatus`
(GPS tag 9) is `V`, the measurement is void, so latitude, longitude and altitude are left absent
rather than placing the image at 0°N 0°E. The DJI `AbsoluteAltitude` is likewise absent when
`drone-dji:GpsStatus` is `Invalid`. The pitch of senseFly, Sentera and Parrot cameras is converted
to the DJI convention, where -90 looks straight down.

### Camera calibration and lens distortion (`Calibration`, `Distortion`)

These come from DJI XMP, which is read when `Make` is `DJI` or the XMP is about `"DJI Meta Data"`.

| Field | XMP |
| --- | --- |
| `Calibration.FocalLength`, `OpticalCenterX`, `OpticalCenterY` | `drone-dji:CalibratedFocalLength`, `CalibratedOpticalCenterX`, `CalibratedOpticalCenterY`, in pixels |
| `Distortion.DewarpFlag` | `drone-dji:DewarpFlag`: 0 for a raw, distorted image; 1 once undistorted |
| `Distortion.K1`, `K2`, `P1`, `P2`, `K3` | `drone-dji:DewarpData`, `"date;Fx,Fy,Cx,Cy,K1,K2,P1,P2,K3"` |

### Spherical panoramas (`GPano`)

From the Google [spherical metadata](https://developers.google.com/streetview/spherical-metadata) XMP.

| Field | XMP |
| --- | --- |
| `ProjectionType` | `GPano:ProjectionType`, as written; `isEquirectangular()` tests it. The numeric `EXIFInfo::ProjectionType` is derived from it: 1 for perspective, 2 for equirectangular |
| `PoseHeadingDegrees`, `PosePitchDegrees`, `PoseRollDegrees` | `GPano:PoseHeadingDegrees`, `PosePitchDegrees`, `PoseRollDegrees` |
| `CroppedAreaImageWidthPixels`, `CroppedAreaImageHeightPixels` | `GPano:CroppedAreaImageWidthPixels`, `CroppedAreaImageHeightPixels` |
| `FullPanoWidthPixels`, `FullPanoHeightPixels` | `GPano:FullPanoWidthPixels`, `FullPanoHeightPixels` |
| `CroppedAreaLeftPixels`, `CroppedAreaTopPixels` | `GPano:CroppedAreaLeftPixels`, `CroppedAreaTopPixels` |

### Motion photos (`MicroVideo`)

From the Google Camera XMP of photos that carry a short video after the image.

| Field | XMP |
| --- | --- |
| `HasMicroVideo`, `MicroVideoVersion`, `MicroVideoOffset` | `GCamera:MicroVideo`, `MicroVideoVersion`, `MicroVideoOffset`: the legacy format, with the video's offset from the end of the file |
| `HasMotionPhoto`, `MotionPhotoLength`, `MotionPhotoMime` | `GCamera:MotionPhoto`, and the length and mime type of the first video item in its `Container:Directory` |

### Embedded images (`MPImages`)

The images listed by the Multi-Picture Format index; see
[Images stored after the main one](#images-stored-after-the-main-one).

## Absent tags vs tags that are legitimately zero

All data fields are zero-initialised, so a value of `0` on its own does not say whether the tag
was missing from the file or whether the camera really wrote a `0`. `HasField()` answers that,
and `GetFields()` lists everything that was found:

```cpp
	TinyEXIF::EXIFInfo imageEXIF(istream);

	// ISOSpeedRatings == 0 alone is ambiguous, this is not
	if (imageEXIF.HasField(TinyEXIF::FIELD_ID_ISOSpeedRatings))
		std::cout << "ISO " << imageEXIF.ISOSpeedRatings << "\n";
	else
		std::cout << "ISO not recorded by the camera\n";

	// list every tag that was present
	for (TinyEXIF::FieldID id: imageEXIF.GetFields())
		std::cout << TinyEXIF::FieldName(id) << "\n";
```

There is one `FieldID` enumerator per data field, named `FIELD_ID_` followed by the path of the
member it fills, e.g. `FIELD_ID_GeoLocation_Altitude` for `GeoLocation.Altitude`; `FieldName()`
returns that path as a string. A field counts as present when the tag carrying it was parsed
successfully, from EXIF or from XMP; a tag that is present but malformed does not count.
That includes any floating point value that is NaN or infinite, or converts to one, so every
floating point field holds a finite number or is absent.

Before 1.1.0, the only way to tell was a sentinel value, and those remain: several fields start
out as `DBL_MAX` or `UINT32_MAX` rather than 0, and these accessors test for them, or for 0:
`GeoLocation.hasLatLon()`, `hasAltitude()`, `hasRelativeAltitude()`, `hasOrientation()`,
`hasSpeed()`, `hasAccuracy()`; `Calibration.hasCalibration()`; `Distortion.hasDewarpFlag()`,
`hasDistortion()`; and `GPano.hasPoseHeadingDegrees()` and the other `GPano.hasXxx()`.

Run the demo with `TinyEXIFdemo <image_file> --fields` to print the list for a file.

This API was added in 1.1.0 and can be feature-gated with the `TINYEXIF_VERSION` macro:

```cpp
	#if TINYEXIF_VERSION >= 10100
	// TinyEXIF::EXIFInfo::HasField() is available
	#endif
```

## Images stored after the main one

A JPEG can carry more images after its own, listed by a Multi-Picture Format (MPF, CIPA DC-007)
index: the preview of a large panorama, the second view of a stereo (`.MPO`) image, an HDR gain
map. `MPImages` lists them in the index order, with the type, offset and length of each.
The first entry is normally the image itself. TinyEXIF only locates them and never reads them,
so their `Offset` and `Length` come straight from the file and must be checked against its
size before use:

```cpp
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	const uint64_t fileSize = (uint64_t)file.tellg();
	file.seekg(0);
	TinyEXIF::EXIFInfo imageEXIF(file);
	for (const TinyEXIF::EXIFInfo::MPImage_t& image: imageEXIF.MPImages) {
		if (!image.isLargeThumbnail() || image.Offset > fileSize || image.Length > fileSize - image.Offset)
			continue;
		std::vector<uint8_t> preview(image.Length);
		file.clear();
		file.seekg((std::streamoff)image.Offset);
		if (file.read((char*)preview.data(), preview.size())) {
			// a complete JPEG, which may carry metadata of its own
			TinyEXIF::EXIFInfo previewEXIF(preview.data(), image.Length);
		}
	}
```

| `MPImage_t` member | Meaning |
| --- | --- |
| `Type` | MP type code: 0x010001 to 0x010005 a large thumbnail (preview), 0x020001 a panorama frame, 0x020002 one view of a stereo image, 0x020003 a multi-angle view, 0x030000 the primary image, 0x040000 an original preservation image, 0x050000 a gain map |
| `Flags` | 4: representative image, 8: dependent child image, 16: dependent parent image |
| `Format` | 0: JPEG, the only format defined |
| `Offset`, `Length` | where the image starts, in bytes from the start of the JPEG, and its size |

## Untrusted files

TinyEXIF is written to parse files that it did not create:

- Every offset and length read from the file is checked against the buffer before use, in 64-bit
  arithmetic where adding them could wrap, and no allocation is sized by the file beyond what it
  actually contains.
- A malformed value is left absent rather than guessed: every floating point field is finite, an
  XMP number must be the whole property text, and a value out of its field's range is dropped,
  or clamped where `TinyEXIF.h` says so (ISO, and XMP integers beyond 32 bits).
- A crafted regression corpus under `Samples/fuzz/` pins each of these, and CI runs the whole corpus
  under AddressSanitizer and UndefinedBehaviorSanitizer, on a big-endian CPU, and with gcc warnings
  as errors. A libFuzzer harness is in `fuzz/`.

See [SECURITY.md](SECURITY.md) to report a vulnerability.

## Building

**As source files.** Add `TinyEXIF.h` and `TinyEXIF.cpp` to your project; they need C++11. Link
TinyXML2, or define `TINYEXIF_NO_XMP_SUPPORT` to build without XMP support and without the
dependency; EXIF and MPF parsing are unaffected.

**With CMake** (3.15 or later), which always builds with TinyXML2, found through
`find_package(tinyxml2 CONFIG)`:

```sh
cmake -S . -B build -DBUILD_SHARED_LIBS=OFF
cmake --build build
cmake --install build
```

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `ON` | build a shared rather than a static library |
| `BUILD_DEMO` | `ON` | build the `TinyEXIFdemo` program |
| `LINK_CRT_STATIC_LIBS` | `OFF` | with MSVC, link the static C runtime |
| `BUILD_FUZZER` | `OFF` | build the libFuzzer harness, Clang only; see [fuzz/README.md](fuzz/README.md) |

With [vcpkg](https://vcpkg.io), which provides TinyXML2, set `VCPKG_ROOT` and use the preset:
`cmake --preset vcpkg`, then `cmake --build --preset vcpkg`.

An installed TinyEXIF is found by CMake as a package:

```cmake
find_package(TinyEXIF CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE TinyEXIF::TinyEXIF)
```

On Windows, a program using TinyEXIF as a DLL needs `TINYEXIF_IMPORT` defined, which the CMake
target does for you.

**Version.** `TINYEXIF_VERSION_STRING` (`"1.2.0"`) and `TINYEXIF_VERSION`
(major × 10000 + minor × 100 + patch, so `10200`) gate features added in later versions.

**Binary compatibility.** 1.2.0 adds members to `EXIFInfo` and requires consumers to rebuild
with the new header. The shared library uses ABI version 2 (`libTinyEXIF.so.2` on Linux,
`libTinyEXIF.2.dylib` on macOS, `TinyEXIF-2.dll` with MSVC on Windows) to keep older binaries from
loading an incompatible layout. The CMake target remains `TinyEXIF::TinyEXIF`.

## Testing

`TestSamples.py` runs the demo on every sample under `Samples/` and compares its output with the
checked-in `.expected` baseline next to each one:

```sh
python3 TestSamples.py --binary build/TinyEXIFdemo
```

`--update` rewrites the baselines, for a change that is meant to alter the output. The crafted
samples under `Samples/fuzz/` are generated by `Samples/fuzz/make_samples.py`, which documents what
each one pins.

## License

MIT [License](https://github.com/cdcseacave/TinyEXIF/blob/master/LICENSE)

Copyright (c) 2025 cdcseacave

## Acknowledgments

Forked from [easyexif](https://github.com/mayanklahiri/easyexif) library (2013 version) of Mayank Lahiri (<mlahiri@gmail.com>); see [LICENSE.easyexif](https://github.com/cdcseacave/TinyEXIF/blob/master/LICENSE.easyexif) for its terms.
