#ifndef _INC_MMSYSTEM
#define _INC_MMSYSTEM

typedef UINT MMRESULT;
typedef UINT        MMVERSION;  /* major (high byte), minor (low byte) */
typedef UINT       *LPUINT;
#define MAXPNAMELEN      32     /* max product name length (including NULL) */

DECLARE_HANDLE(HWAVE);
DECLARE_HANDLE(HWAVEIN);
DECLARE_HANDLE(HWAVEOUT);
#ifndef _WIN32_VXD
typedef HWAVEIN *LPHWAVEIN;
typedef HWAVEOUT *LPHWAVEOUT;
#endif
typedef struct wavehdr_tag {
    LPSTR       lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    DWORD       dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    struct wavehdr_tag FAR *lpNext;     /* reserved for driver */
    DWORD       reserved;               /* reserved for driver */
} PACKED WAVEHDR, *LPWAVEHDR;

#define MM_WOM_OPEN         0x3BB           /* waveform output */
#define MM_WOM_CLOSE        0x3BC
#define MM_WOM_DONE         0x3BD

#define MM_WIM_OPEN         0x3BE           /* waveform input */
#define MM_WIM_CLOSE        0x3BF
#define MM_WIM_DATA         0x3C0

/* wave callback messages */
#define WOM_OPEN        MM_WOM_OPEN
#define WOM_CLOSE       MM_WOM_CLOSE
#define WOM_DONE        MM_WOM_DONE
#define WIM_OPEN        MM_WIM_OPEN
#define WIM_CLOSE       MM_WIM_CLOSE
#define WIM_DATA        MM_WIM_DATA

/* flags for dwFlags parameter in waveOutOpen() and waveInOpen() */
#define  WAVE_FORMAT_QUERY         0x0001
#define  WAVE_ALLOWSYNC            0x0002
#if(WINVER >= 0x0400)
#define  WAVE_MAPPED               0x0004
#define  WAVE_FORMAT_DIRECT        0x0008
#define  WAVE_FORMAT_DIRECT_QUERY  (WAVE_FORMAT_QUERY | WAVE_FORMAT_DIRECT)
#endif /* WINVER >= 0x0400 */

#define CALLBACK_TYPEMASK   0x00070000l    /* callback type mask */
#define CALLBACK_NULL       0x00000000l    /* no callback */
#define CALLBACK_WINDOW     0x00010000l    /* dwCallback is a HWND */
#define CALLBACK_TASK       0x00020000l    /* dwCallback is a HTASK */
#define CALLBACK_FUNCTION   0x00030000l    /* dwCallback is a FARPROC */
#ifdef _WIN32
#define CALLBACK_THREAD     (CALLBACK_TASK)/* thread ID replaces 16 bit task */
#define CALLBACK_EVENT      0x00050000l    /* dwCallback is an EVENT Handle */
#endif
/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1
/* OLD general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} PACKED WAVEFORMAT, *LPWAVEFORMAT;

/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
} PACKED PCMWAVEFORMAT, *LPPCMWAVEFORMAT;

#ifndef UNICODE_ONLY
typedef struct tagWAVEOUTCAPSA {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVERSION vDriverVersion;      /* version of the driver */
    CHAR    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    WORD    wChannels;             /* number of sources supported */
    WORD    wReserved1;            /* packing */
    DWORD   dwSupport;             /* functionality supported by driver */
} PACKED WAVEOUTCAPSA, *PWAVEOUTCAPSA, *NPWAVEOUTCAPSA, *LPWAVEOUTCAPSA;
#endif /*!UNICODE_ONLY */
#ifndef ANSI_ONLY
typedef struct tagWAVEOUTCAPSW {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVERSION vDriverVersion;      /* version of the driver */
    WCHAR   szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    WORD    wChannels;             /* number of sources supported */
    WORD    wReserved1;            /* packing */
    DWORD   dwSupport;             /* functionality supported by driver */
} PACKED WAVEOUTCAPSW, *PWAVEOUTCAPSW, *NPWAVEOUTCAPSW, *LPWAVEOUTCAPSW;
#endif /*!ANSI_ONLY */
#ifdef UNICODE
typedef WAVEOUTCAPSW WAVEOUTCAPS;
typedef PWAVEOUTCAPSW PWAVEOUTCAPS;
typedef NPWAVEOUTCAPSW NPWAVEOUTCAPS;
typedef LPWAVEOUTCAPSW LPWAVEOUTCAPS;
#else
typedef WAVEOUTCAPSA WAVEOUTCAPS;
typedef PWAVEOUTCAPSA PWAVEOUTCAPS;
typedef NPWAVEOUTCAPSA NPWAVEOUTCAPS;
typedef LPWAVEOUTCAPSA LPWAVEOUTCAPS;
#endif /* UNICODE */

/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
				    /* extra information (after cbSize) */
} PACKED WAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;


/* MMTIME data structure */
typedef struct mmtime_tag
{
    UINT            wType;      /* indicates the contents of the union */
    union
    {
	DWORD       ms;         /* milliseconds */
	DWORD       sample;     /* samples */
	DWORD       cb;         /* byte count */
	DWORD       ticks;      /* ticks in MIDI stream */

	/* SMPTE */
	struct
	{
	    BYTE    hour;       /* hours */
	    BYTE    min;        /* minutes */
	    BYTE    sec;        /* seconds */
	    BYTE    frame;      /* frames  */
	    BYTE    fps;        /* frames per second */
	    BYTE    dummy;      /* pad */
	    BYTE    pad[2];
	} smpte;

	/* MIDI */
	struct
	{
	    DWORD songptrpos;   /* song pointer position */
	} midi;
    } u;
} PACKED MMTIME, *LPMMTIME;

#define WINMMAPI extern

/* waveform audio function prototypes */
WINMMAPI UINT WINAPI waveOutGetNumDevs(void); 

                           
#ifndef UNICODE_ONLY
WINMMAPI MMRESULT WINAPI waveOutGetDevCapsA(UINT uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc);
#endif /*!UNICODE_ONLY */
#ifndef ANSI_ONLY
WINMMAPI MMRESULT WINAPI waveOutGetDevCapsW(UINT uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc);
#endif /*!ANSI_ONLY */
#ifdef UNICODE
#define waveOutGetDevCaps  waveOutGetDevCapsW
#else
#define waveOutGetDevCaps  waveOutGetDevCapsA
#endif /* !UNICODE */
#ifndef UNICODE_ONLY
WINMMAPI MMRESULT WINAPI waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText);
#endif //!UNICODE_ONLY
#ifndef ANSI_ONLY
WINMMAPI MMRESULT WINAPI waveOutGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
#endif //!ANSI_ONLY
#ifdef UNICODE
#define waveOutGetErrorText  waveOutGetErrorTextW
#else
#define waveOutGetErrorText  waveOutGetErrorTextA
#endif // !UNICODE
WINMMAPI MMRESULT WINAPI waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID,
    LPCWAVEFORMATEX pwfx, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);

WINMMAPI MMRESULT WINAPI waveOutClose(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveOutPause(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutRestart(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutReset(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutBreakLoop(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt);
WINMMAPI MMRESULT WINAPI waveOutGetPitch(HWAVEOUT hwo, LPDWORD pdwPitch);
WINMMAPI MMRESULT WINAPI waveOutSetPitch(HWAVEOUT hwo, DWORD dwPitch);
WINMMAPI MMRESULT WINAPI waveOutGetPlaybackRate(HWAVEOUT hwo, LPDWORD pdwRate);
WINMMAPI MMRESULT WINAPI waveOutSetPlaybackRate(HWAVEOUT hwo, DWORD dwRate);
WINMMAPI MMRESULT WINAPI waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID);


#endif /* _INC_MMSYSTEM */
