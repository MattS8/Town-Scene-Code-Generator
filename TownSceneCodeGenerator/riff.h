//---------------------------------------------------------------------------
#ifndef riffH
#define riffH
#include <windows.h>
#include <mmsystem.h>   // für WAVEFORMATEX usw.
//#include <mmreg.h>      // für WAVE_FORMAT_IEEE_FLOAT etc.
#include <stdio.h>
#include <iostream>     // std::cout
//#include <stdlib.h>
//#include <string.h>

// Definitions for RIFF file I/O

#pragma pack(1)

// Strukturen für die Cue-List

// CUECHUNK ist ein Sub-Chunk des RIFF-Chunks:

struct CUECHUNK
{
	DWORD chunkID;                // Chunk-ID = "cue "
	DWORD chunkSize;              // Chunk-Größe
	DWORD dwCuePoints;            // Anzahl (n) der nachfolgenden CUEPOINT-Strukturen
};

struct CUEPOINT                 // (n) solcher Strukturen folgen hinter CUECHUNK.
{
	DWORD dwIdentifier;           // = Nummer der Cue (1...n)
	DWORD dwPosition;             // = Start-Sample ("play order")
	DWORD fccChunk;               // = "data"
	DWORD dwChunkStart;           // = 0
	DWORD dwBlockStart;           // = 0
	DWORD dwSampleOffset;         // = Start-Sample
};

// Die folgenden Chunks sind Sub-Chunks des LIST-Chunks:

struct CUELABELTEXTCHUNK
{
	DWORD chunkID;                // Chunk-ID = "ltxt"
	DWORD chunkSize;              // Chunk-Größe
	DWORD dwIdentifier;           // = Nummer der Cue (1...n)
	DWORD dwSampleLength;         // = Cue-Länge in Samples
	DWORD dwPurpose;              // = "rgn ", "beat", "trak", "indx", etc.
	DWORD dw1;                    // = 0
	DWORD dw2;                    // = 0
};

struct CUETEXTCHUNK
{
	DWORD chunkID;                // Chunk-ID = "labl" bzw. "note"
	DWORD chunkSize;              // Chunk-Größe
	DWORD dwIdentifier;           // Nummer der Cue (1...n)
	char szText[256];             // Text-String (Label bzw. Description)
};

struct PLAYLISTCHUNK
{
	DWORD chunkID;                // Chunk-ID = "plst"
	DWORD chunkSize;              // Chunk-Größe
	DWORD dwSegments;             // Anzahl (n) der nachfolgenden PLAYLISTITEM-Strukturen
};

struct PLAYLISTITEM             // (n) solcher Strukturen folgen hinter PLAYLISTCHUNK.
{
	DWORD dwIdentifier;           // = Nummer der Cue (1...n)
	DWORD dwLength;               // = Cue-Länge in Samples
	DWORD dwRepeats;              // = Anzahl der Wiederholungen (Loops)
};

#pragma pack()


// Funktionen zum Parsen von RIFF-Files

std::string FOURCCToString(DWORD dwFOURCC);
DWORD StringToFOURCC(std::string strFOURCC);
void RIFFError(std::string strChunkName);
int FindChunk(FILE *fp, LPMMCKINFO pChunk, LPMMCKINFO pParentChunk);
int SeekBehindChunk(FILE *fp, LPMMCKINFO pChunk);

//---------------------------------------------------------------------------
#endif

