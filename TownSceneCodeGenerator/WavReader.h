#pragma once
#ifndef WAVREADER
#define WAVREADER

#include <fstream>
#include <math.h>

#include <iostream>     // std::cout
#include <sstream>
#include "TownSceneCodeGenerator.h"
#include <iostream>     // std::cout
#include "riff.h"
#include "mmreg.h"
#include <algorithm> 
#include <cctype>
#include <locale>
#include <windows.h>


// -------- WAV File Reader --------
// From CueListTool by S.B.
// Ported by Matt Steinhardt

#define MAXCUES 1000            // 1000 cues should be enough ...
#define DEFAULT_SAMPLERATE 44100

#define MFS_FRM 1
#define MFS_SEC 2
#define MFS_MIN 3
#define MFS_HRS 4
#define MFS_SMP 5

// Array for cues of the input WAV file

struct CUEARRAY                 // ATTENTION: When expanding this structure, adjust SortCueArray ()!
{
	int StartSample;              // First sample within the wave data
	int nSamples;                 // Number of samples for this segment
	DWORD ID;                     // Number of the cue
	std::string Label;
	std::string Description;
	std::string Type;
};

// Forward declarations

int					CompareCueArrayElements(CUEARRAY* pElement1, CUEARRAY* pElement2);
void				SwapCueArrayElements(CUEARRAY* pElement1, CUEARRAY* pElement2);
std::string			MakeFormatString(int nFormatStringType, bool bFractional);
bool				CarryTimeString(std::string strTime, int nMaxValue);

int nDataChunkSizeDiff;         // Difference in the size of the data chunks of the original
								// Wav file opposite the WavPack file
CUEARRAY* CueArray = new CUEARRAY[MAXCUES];
int nCues;                      // Number of existing cues
int nCueListSamplesPerSec;      // Samples per second of CueList

int nFilePointerCueSubchunk;    // Shows behind the wave data
int nMaxCues = MAXCUES;
std::string strCueSheetCDTitle;

// Format options

int nFormatTimeLeadingDigitsHrs = 1;
int nFormatTimeFractionalDigitsHrs = 10;
int nFormatTimeLeadingDigitsMin = 1;
int nFormatTimeFractionalDigitsMin = 4;
int nFormatTimeLeadingDigitsSec = 2;
int nFormatTimeFractionalDigitsSec = 3;
int nFormatTimeLeadingDigitsFrm = 0;
int nFormatTimeFractionalDigitsFrm = 1;
int nFormatTimeLeadingDigitsSmp = 1;
int nFormatTimeFractionalDigitsSmp = 0;

// Information about the selected audio file

std::string strAudioFileName;
bool bAudioFileIsValid;
int nAudioFileSamples;
int nAudioFileSamplesPerSec;
WAVEFORMATEX WaveFmt;

// Variables replacing TOpenDialog

std::string fileName;

//---------------------------------------------------------------------------

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

//---------------------------------------------------------------------------

void RIFFError(std::string strChunkName)
{
	std::ostringstream ss;
	ss << "Cannot find '" << strChunkName << "' chunk.";
	OutputLogStr.append(ss.str()).append("\r\n");
}
//---------------------------------------------------------------------------

std::string FOURCCToString(DWORD dwFOURCC)
{
	std::string strFOURCC = "";
	std::ostringstream ss;

	for (int i = 0; i < 4; i++)
	{
		int nDigit = (dwFOURCC >> (8 * i)) & 0xFF;
		ss << (char)nDigit;
		strFOURCC += ss.str();
	}

	rtrim(strFOURCC);
	return strFOURCC;
}
//---------------------------------------------------------------------------

DWORD StringToFOURCC(std::string strFOURCC)
{
	DWORD dwFOURCC = 0;
	strFOURCC += "    ";
	const char* pszFOURCC = strFOURCC.c_str();

	for (int i = 3; i >= 0; i--)
	{
		dwFOURCC *= 0x100;
		dwFOURCC += pszFOURCC[i];
	}
	return dwFOURCC;
}


//---------------------------------------------------------------------------
//This function is essentially equivalent to mmioDescend(), but with the following differences :
//-Also chunks with odd chunksize and missing padbyte are found.
//- The uFlags parameter is omitted; instead, fill pChunk->ckid(as with subchunks).

int FindChunk(FILE *fp, LPMMCKINFO pChunk, LPMMCKINFO pParentChunk)
{
	DWORD dwChunkId = pChunk->ckid;

	int nBytesToRead = 8;
	if (dwChunkId == StringToFOURCC("RIFF") || dwChunkId == StringToFOURCC("LIST")) nBytesToRead = 12;

	int nBytesMax = 0x7fffffff;
	if (pParentChunk) nBytesMax = pParentChunk->cksize + pParentChunk->dwDataOffset - ftell(fp);

	int nBytesReadSoFar = 0;

	do
	{
		if (nBytesReadSoFar + nBytesToRead > nBytesMax) return -1;
		size_t nBytesRead = fread(pChunk, 1, nBytesToRead, fp);
		if (nBytesRead != nBytesToRead) return -1;
		if ((pChunk->ckid & 0xff) == 0)     // 1. Byte = Padbyte?
		{
			fseek(fp, 1 - nBytesToRead, SEEK_CUR);    // File-Pointer zurücksetzen + 1 Byte vor
			nBytesReadSoFar++;
			if (nBytesReadSoFar + nBytesToRead > nBytesMax) return -1;
			nBytesRead = fread(pChunk, 1, nBytesToRead, fp);  // nochmal lesen
			if (nBytesRead != nBytesToRead) return -1;
		}
		nBytesReadSoFar += nBytesRead;
		if (pChunk->ckid == dwChunkId)
		{
			pChunk->dwDataOffset = ftell(fp) + 8 - nBytesToRead;
			return 0;
		}
		if (pChunk->cksize <= 0) return -1; // unsinniger Wert?
		int nBytesToSkip = pChunk->cksize + 8 - nBytesToRead;
		if (nBytesReadSoFar + nBytesToSkip > nBytesMax) return -1;
		if (fseek(fp, nBytesToSkip, SEEK_CUR) != 0) return -1;
		nBytesReadSoFar += nBytesToSkip;
	} while (true);
}

//---------------------------------------------------------------------------
//This function is basically similar to mmioAscend(), but with the following differences :
//-It can not be used to write chunks(see mmioCreateChunk)

int SeekBehindChunk(FILE *fp, LPMMCKINFO pChunk)
{
	return fseek(fp, pChunk->cksize + pChunk->dwDataOffset - ftell(fp), SEEK_CUR);
}

//---------------------------------------------------------------------------

bool GetWavFileData(std::string strFileName, int& nSamples, int& nSamplesPerSec)
{
	bool nSuccess = false;
	FILE *fp;
	bool bFileOpen = false;
	MMCKINFO RiffChunkInfo; // RIFF-Chunk
	MMCKINFO FormatChunkInfo; // Format-Subchunk (Wave-Parameter)
	MMCKINFO DataChunkInfo; // Data-Subchunk (Wave-Audio-Daten)
	WAVEFORMATEX WaveFmt;

	nSamples = 0;
	nSamplesPerSec = 0;

	do
	{
		errno_t err = fopen_s(&fp, strFileName.c_str(), "rb");
		//fp = fopen(strFileName.c_str(), "rb");
		if (err != 0) break;
		bFileOpen = true;

		// Find the RIFF chunk.
		memset(&RiffChunkInfo, 0, sizeof(MMCKINFO));
		RiffChunkInfo.ckid = StringToFOURCC("RIFF");
		if (FindChunk(fp, &RiffChunkInfo, NULL)) break;

		// Descend into the format chunk.
		memset(&FormatChunkInfo, 0, sizeof(MMCKINFO));
		FormatChunkInfo.ckid = StringToFOURCC("fmt");
		if (FindChunk(fp, &FormatChunkInfo, &RiffChunkInfo)) break;

		// Read the wave format.
		memset(&WaveFmt, 0, sizeof(WAVEFORMATEX));
		int nBytesToRead = sizeof(WAVEFORMATEX) < FormatChunkInfo.cksize ? sizeof(WAVEFORMATEX) : FormatChunkInfo.cksize;
		fread((char*)&WaveFmt, 1, nBytesToRead, fp);
		if (WaveFmt.wFormatTag != WAVE_FORMAT_PCM && WaveFmt.wFormatTag != WAVE_FORMAT_IEEE_FLOAT) break;

		// Ascend out of the format chunk.
		if (SeekBehindChunk(fp, &FormatChunkInfo)) break;

		// Descend into the data chunk.
		memset(&DataChunkInfo, 0, sizeof(MMCKINFO));
		DataChunkInfo.ckid = StringToFOURCC("data");
		if (FindChunk(fp, &DataChunkInfo, &RiffChunkInfo)) break;

		int nBytesPerSample = (WaveFmt.wBitsPerSample / 8) * WaveFmt.nChannels;
		if (nBytesPerSample > 0) nSamples = DataChunkInfo.cksize / nBytesPerSample;
		nSamplesPerSec = WaveFmt.nSamplesPerSec;

		nSuccess = (nSamples > 0 && nSamplesPerSec > 0);
	} while (false);

	if (bFileOpen) fclose(fp);;

	return nSuccess;
}

//---------------------------------------------------------------------------

bool CueExists(int iCueArrayIndex)
{
	//if (!FormSettings->CheckBoxEliminateDuplicateCues->Checked) return false;

	int     StartSample = CueArray[iCueArrayIndex].StartSample;
	int     nSamples = CueArray[iCueArrayIndex].nSamples;
	std::string  Label = CueArray[iCueArrayIndex].Label;
	std::string  Description = CueArray[iCueArrayIndex].Description;
	std::string  Type = CueArray[iCueArrayIndex].Type;

	for (int iCue = 0; iCue < iCueArrayIndex; iCue++)
	{
		if (
			CueArray[iCue].StartSample == StartSample &&
			CueArray[iCue].nSamples == nSamples &&
			CueArray[iCue].Label == Label &&
			CueArray[iCue].Description == Description &&
			CueArray[iCue].Type == Type
			) return true;
	}
	return false;
}

//---------------------------------------------------------------------------

int ReadCueListFromRiffFile(std::string strFileName, int nCueArrayOffset, bool bDeleteCueListFromAudioFile/* = false*/)
{
	MMCKINFO RiffChunkInfo; // RIFF-Chunk
	MMCKINFO FormatChunkInfo; // Format-Subchunk (Wave-Parameter)
	MMCKINFO DataChunkInfo; // Data-Subchunk (Wave-Audio-Daten)
	MMCKINFO CueChunkInfo; // Cue-Subchunk (Startsample)
	MMCKINFO ListChunkInfo; // LIST-Chunk
	MMCKINFO LtxtChunkInfo; // Cue-Länge
	MMCKINFO LablChunkInfo; // Cue-Label
	MMCKINFO NoteChunkInfo; // Cue-Description
	CUEPOINT CuePoint;
	CUELABELTEXTCHUNK CueLtxtChunk;
	CUETEXTCHUNK CueTextChunk;
	std::string strMsg;
	FILE *fp;
	errno_t err = fopen_s(&fp, strFileName.c_str(), "rb");
	if (err != 0)
	{
		strMsg = "";
		strMsg.append("Error: Can't open WAV file ").append(strFileName).append(" for reading.");
		OutputLogStr.append(strMsg).append("\r\n");
		//MessageBox(Handle, strMsg.c_str(), "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
		return -1;
	}

	// Find the RIFF chunk.
	memset(&RiffChunkInfo, 0, sizeof(MMCKINFO));
	RiffChunkInfo.ckid = StringToFOURCC("RIFF");
	if (FindChunk(fp, &RiffChunkInfo, NULL))
	{
		fclose(fp);
		RIFFError("RIFF");
		return -1;
	}

	// Descend into the format chunk.
	memset(&FormatChunkInfo, 0, sizeof(MMCKINFO));
	FormatChunkInfo.ckid = StringToFOURCC("fmt");
	if (FindChunk(fp, &FormatChunkInfo, &RiffChunkInfo))
	{
		fclose(fp);
		RIFFError("fmt");
		return -1;
	}

	// Read the wave format.
	memset(&WaveFmt, 0, sizeof(WAVEFORMATEX));
	int nBytesToRead = sizeof(WAVEFORMATEX) < FormatChunkInfo.cksize ? sizeof(WAVEFORMATEX) : FormatChunkInfo.cksize;
	fread((char*)&WaveFmt, 1, nBytesToRead, fp);

	if (WaveFmt.wFormatTag != WAVE_FORMAT_PCM && WaveFmt.wFormatTag != WAVE_FORMAT_IEEE_FLOAT)
	{
		fclose(fp);
		strMsg = "Error: This WAV file is not a standard Windows PCM or IEEE754 32-bit/64-bit (MSVC++ float/double) file.";
		OutputLogStr.append(strMsg).append("\r\n");
		//MessageBox(Handle, "This WAV file is not a standard Windows PCM or IEEE754 32-bit/64-bit (MSVC++ float/double) file.", "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
		return -1;
	}

	// Ascend out of the format chunk.
	if (SeekBehindChunk(fp, &FormatChunkInfo))
	{
		fclose(fp);
		RIFFError("fmt");
		return -1;
	}

	// Descend into the data chunk.
	memset(&DataChunkInfo, 0, sizeof(MMCKINFO));
	DataChunkInfo.ckid = StringToFOURCC("data");
	if (FindChunk(fp, &DataChunkInfo, &RiffChunkInfo))
	{
		fclose(fp);
		RIFFError("data");
		return -1;
	}

	// Set the file pointer behind the data.
  //  fseek(fp, DataChunkInfo.cksize, SEEK_CUR);
  //  nFilePointerCueSubchunk = ftell(fp);
	// Neu: DataChunkInfo.cksize neu berechnen, damit auch WavPack-Dateien gelesen werden können:
	int nCurrentFilePos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	int nFileSize = ftell(fp);
	nDataChunkSizeDiff = DataChunkInfo.cksize;
	DataChunkInfo.cksize = nFileSize + DataChunkInfo.cksize - RiffChunkInfo.cksize - 8;
	nDataChunkSizeDiff -= DataChunkInfo.cksize;
	fseek(fp, nCurrentFilePos + DataChunkInfo.cksize, SEEK_SET);
	nFilePointerCueSubchunk = ftell(fp);

	// Falls die WAV-Datei eine Cue-List enthält, dann diese ins Array lesen:
	int nCuesInFile = 0;

	do // "Pseudo"-Schleife, um im Fehlerfalle mit 'break' hinter die Schleife zu springen. ;-)
	{
		// Ascend out of the data chunk.
		if (SeekBehindChunk(fp, &DataChunkInfo)) break;

		// Descend into the cue chunk.
		memset(&CueChunkInfo, 0, sizeof(MMCKINFO));
		CueChunkInfo.ckid = StringToFOURCC("cue");
		if (FindChunk(fp, &CueChunkInfo, &RiffChunkInfo)) break;

		// Anzahl Cue-Positions nach nCuesInFile lesen
		fread((char*)&nCuesInFile, sizeof(DWORD), 1, fp); // Anzahl Cuepoints
		if (nCuesInFile == 0) break; // Keine Cues gefunden

		// Wenn die Cue-List aus der WAV-Datei gelöscht werden soll, dann nur die Anzahl der Cues zurückgeben.
		if (bDeleteCueListFromAudioFile) break; //

		if (sizeof(DWORD) + nCuesInFile * sizeof(CUEPOINT) != CueChunkInfo.cksize)
		{
			strMsg = "Number of found Cue points doesn't match size of cue chunk.";
			OutputLogStr.append(strMsg).append("\r\n");
			//MessageBox(Handle, "Number of found Cue points doesn't match size of cue chunk.", "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
			break;
		}

		if (nCueArrayOffset > 0 && nCuesInFile + nCueArrayOffset > nMaxCues)
		{
			strMsg = "";
			strMsg.append("Maximum number of Cue points (").append(std::to_string(nMaxCues)).append(" exceeded by ").append(std::to_string(nCuesInFile + nCueArrayOffset - nMaxCues));
			OutputLogStr.append(strMsg).append("\r\n");
			//MessageBox(Handle, strMsg.c_str(), "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
			fclose(fp);
			return -1;
		}

		int iCueRead;
		for (iCueRead = 0; iCueRead < nCuesInFile && iCueRead < nMaxCues - nCueArrayOffset; iCueRead++)
		{
			memset(&CuePoint, 0, sizeof(CUEPOINT));
			fread((char*)&CuePoint, sizeof(CUEPOINT), 1, fp); // Cuepoints in Struktur einlesen
			CueArray[iCueRead + nCueArrayOffset].StartSample = CuePoint.dwSampleOffset;
			CueArray[iCueRead + nCueArrayOffset].ID = CuePoint.dwIdentifier;
			CueArray[iCueRead + nCueArrayOffset].nSamples = 0;
			CueArray[iCueRead + nCueArrayOffset].Label = "";
			CueArray[iCueRead + nCueArrayOffset].Description = "";
			CueArray[iCueRead + nCueArrayOffset].Type = "";
		}

		if (iCueRead < nCuesInFile)
		{
			strMsg = "";
			strMsg.append("Only ").append(std::to_string(iCueRead)).append(" of ").append(std::to_string(nCuesInFile)).append(" could be reead.");
			OutputLogStr.append(strMsg).append("\r\n");
			//MessageBox(Handle, strMsg.c_str(), "Warning", MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
			nCuesInFile = iCueRead;
		}

		// Ascend out of the cue chunk.
		if (SeekBehindChunk(fp, &CueChunkInfo)) break;

		// Find the LIST chunk.
		memset(&ListChunkInfo, 0, sizeof(MMCKINFO));
		ListChunkInfo.ckid = StringToFOURCC("LIST");
		if (FindChunk(fp, &ListChunkInfo, NULL))
		{
			nCuesInFile = 0;
			break;
		}

		do // Read cue lengths:
		{
			// Descend into the ltxt chunk.
			memset(&LtxtChunkInfo, 0, sizeof(MMCKINFO));
			LtxtChunkInfo.ckid = StringToFOURCC("ltxt");
			if (FindChunk(fp, &LtxtChunkInfo, &ListChunkInfo)) break;

			memset(&CueLtxtChunk, 0, sizeof(CUELABELTEXTCHUNK));
			fread((char*)&CueLtxtChunk + 8, LtxtChunkInfo.cksize, 1, fp);

			for (int iCue = 0; iCue < nCuesInFile; iCue++)
			{
				if (CueArray[iCue + nCueArrayOffset].ID > 0 && CueArray[iCue + nCueArrayOffset].ID == CueLtxtChunk.dwIdentifier)
				{
					CueArray[iCue + nCueArrayOffset].nSamples = CueLtxtChunk.dwSampleLength;
					CueArray[iCue + nCueArrayOffset].Type = FOURCCToString(CueLtxtChunk.dwPurpose);
					if (CueArray[iCue + nCueArrayOffset].Type.empty()) CueArray[iCue + nCueArrayOffset].Type = "rgn";
					break;
				}
			}
			// Ascend out of the ltxt chunk.
			if (SeekBehindChunk(fp, &LtxtChunkInfo)) break;
		} while (true);

		fseek(fp, ListChunkInfo.dwDataOffset + 4, SEEK_SET); // Zum Beginn des List-Chunks!

		do // Read cue labels:
		{
			// Descend into the labl chunk.
			memset(&LablChunkInfo, 0, sizeof(MMCKINFO));
			LablChunkInfo.ckid = StringToFOURCC("labl");
			if (FindChunk(fp, &LablChunkInfo, &ListChunkInfo)) break;

			memset(&CueTextChunk, 0, sizeof(CUETEXTCHUNK));
			fread((char*)&CueTextChunk + 8, LablChunkInfo.cksize, 1, fp);

			for (int iCue = 0; iCue < nCuesInFile; iCue++)
			{
				if (CueArray[iCue + nCueArrayOffset].ID > 0 && CueArray[iCue + nCueArrayOffset].ID == CueTextChunk.dwIdentifier)
				{
					CueArray[iCue + nCueArrayOffset].Label = CueTextChunk.szText;
					break;
				}
			}
			// Ascend out of the labl chunk.
			if (SeekBehindChunk(fp, &LablChunkInfo)) break;
		} while (true);

		fseek(fp, ListChunkInfo.dwDataOffset + 4, SEEK_SET); // Zum Beginn des List-Chunks!

		do // Read cue descriptions:
		{
			// Descend into the note chunk.
			memset(&NoteChunkInfo, 0, sizeof(MMCKINFO));
			NoteChunkInfo.ckid = StringToFOURCC("note");
			if (FindChunk(fp, &NoteChunkInfo, &ListChunkInfo)) break;

			memset(&CueTextChunk, 0, sizeof(CUETEXTCHUNK));
			fread((char*)&CueTextChunk + 8, NoteChunkInfo.cksize, 1, fp);

			for (int iCue = 0; iCue < nCuesInFile; iCue++)
			{
				if (CueArray[iCue + nCueArrayOffset].ID > 0 && CueArray[iCue + nCueArrayOffset].ID == CueTextChunk.dwIdentifier)
				{
					CueArray[iCue + nCueArrayOffset].Description = CueTextChunk.szText;
					break;
				}
			}
			// Ascend out of the note chunk.
			if (SeekBehindChunk(fp, &NoteChunkInfo)) break;
		} while (true);
	} while (false); // Go through "loop" only once

	// Close the file
	fclose(fp);

	// If the cue list is to be deleted from the WAV file, then only return the number of cues.
	if (bDeleteCueListFromAudioFile) return nCuesInFile;

	// Set prefix before cue label
	////if (FormSettings->CheckBoxEnablePrefixes->Checked && FormSettings->EditPrefix->Text != "")
	////	for (int iCue = 0; iCue < nCuesInFile; iCue++)
	////		if (CueArray[iCue + nCueArrayOffset].Label.SubString(1, 1) != "<")
	////			CueArray[iCue + nCueArrayOffset].Label = "<" + FormSettings->EditPrefix->Text + "> " + CueArray[iCue + nCueArrayOffset].Label;

	// Remove duplicates from the CueListArray
	for (int iCue = 0; iCue < nCuesInFile; iCue++)
	{
		if (CueExists(iCue + nCueArrayOffset))
		{
			// Move all cues in the array 1 position to the left:
			for (int iCueToMove = iCue; iCueToMove < nCuesInFile - 1; iCueToMove++)
			{
				CueArray[iCueToMove + nCueArrayOffset].StartSample = CueArray[iCueToMove + 1 + nCueArrayOffset].StartSample;
				CueArray[iCueToMove + nCueArrayOffset].nSamples = CueArray[iCueToMove + 1 + nCueArrayOffset].nSamples;
				CueArray[iCueToMove + nCueArrayOffset].Label = CueArray[iCueToMove + 1 + nCueArrayOffset].Label;
				CueArray[iCueToMove + nCueArrayOffset].Description = CueArray[iCueToMove + 1 + nCueArrayOffset].Description;
				CueArray[iCueToMove + nCueArrayOffset].Type = CueArray[iCueToMove + 1 + nCueArrayOffset].Type;
			}
			nCuesInFile--, iCue--;
		}
	}

	return nCuesInFile;
}

//---------------------------------------------------------------------------

void SortCueArray(void)
{
	if (nCues < 2) return;

	// Bubblesort
	for (int iEnde = nCues - 1; iEnde >= 0; iEnde--)
	{
		for (int iElement = 0; iElement < iEnde; iElement++)
		{
			if (CompareCueArrayElements(&CueArray[iElement], &CueArray[iElement + 1]) > 0)
			{
				SwapCueArrayElements(&CueArray[iElement], &CueArray[iElement + 1]);
			}
		}
	}
}

//---------------------------------------------------------------------------

void ReadCueListFromAudioFile(std::string strFileName)
{
	int nCuesRead;

	// Currently only RIFF files implemented (later distinguished by file types).
	if ((nCuesRead = ReadCueListFromRiffFile(strFileName, 0, false)) != -1)
	{
		nCues = nCuesRead;
		SortCueArray();
		nCueListSamplesPerSec = WaveFmt.nSamplesPerSec;
	}
}

//---------------------------------------------------------------------------
// Functions for sorting the cue array:

std::string CueArrayElementToSortString(CUEARRAY* pCueArrayElement)
{
	std::ostringstream ss;
	ss << pCueArrayElement->StartSample << pCueArrayElement->nSamples << pCueArrayElement->Label << pCueArrayElement->Description;
	ss << pCueArrayElement->Type << pCueArrayElement->StartSample << pCueArrayElement->nSamples << pCueArrayElement->Label;
	ss << pCueArrayElement->Description << pCueArrayElement->Type << "\r\n";

	return ss.str();
}

//---------------------------------------------------------------------------

void RemoveNewLine(char *pszLine)
{
	if (strlen(pszLine) > 0) if (pszLine[strlen(pszLine) - 1] == '\n') pszLine[strlen(pszLine) - 1] = 0;
}

//---------------------------------------------------------------------------

bool VerifyCueIndex(int iCue)
{
	bool bOK = true;

	if (iCue >= nMaxCues)
	{
		std::string strTemp = "Maximum number of Cues (";
		strTemp.append(std::to_string(nMaxCues)).append(") reached - can't proceed!");
		OutputLogStr.append(strTemp).append("/r/n");
		bOK = false;
	}
	return bOK;
}

//---------------------------------------------------------------------------

void SwapCueArrayElements(CUEARRAY* pElement1, CUEARRAY* pElement2)
{
	CUEARRAY tmpCueArrayElement;

	tmpCueArrayElement.StartSample = pElement1->StartSample;
	tmpCueArrayElement.nSamples = pElement1->nSamples;
	tmpCueArrayElement.ID = pElement1->ID;
	tmpCueArrayElement.Label = pElement1->Label;
	tmpCueArrayElement.Description = pElement1->Description;
	tmpCueArrayElement.Type = pElement1->Type;

	pElement1->StartSample = pElement2->StartSample;
	pElement1->nSamples = pElement2->nSamples;
	pElement1->ID = pElement2->ID;
	pElement1->Label = pElement2->Label;
	pElement1->Description = pElement2->Description;
	pElement1->Type = pElement2->Type;

	pElement2->StartSample = tmpCueArrayElement.StartSample;
	pElement2->nSamples = tmpCueArrayElement.nSamples;
	pElement2->ID = tmpCueArrayElement.ID;
	pElement2->Label = tmpCueArrayElement.Label;
	pElement2->Description = tmpCueArrayElement.Description;
	pElement2->Type = tmpCueArrayElement.Type;
}

//---------------------------------------------------------------------------

int CompareCueArrayElements(CUEARRAY* pElement1, CUEARRAY* pElement2)
{
	std::string strElement1 = CueArrayElementToSortString(pElement1);
	std::string strElement2 = CueArrayElementToSortString(pElement2);
	return strElement1.compare(strElement2);
}

//---------------------------------------------------------------------------

// Creates a string of chars 'c' with length 'len'
std::string StringOfChar(char c, int len)
{
	std::string strRet = "";

	for (int i = 0; i < len; i++)
	{
		strRet.push_back(c);
	}

	return strRet;
}

//---------------------------------------------------------------------------

std::string FormatTime(double fSec)
{
	std::string strResult;
	std::string strHrs, strMin, strSec, strFrm, strSmp;

	// Calculate the possible values
	double fHrsTotal = fSec / 3600;
	double fMinTotal = fSec / 60;
	double fSecTotal = fSec;
	double fFrmTotal = fSec * 75;
	double fSmpTotal = fSec * (bAudioFileIsValid ? nAudioFileSamplesPerSec : nCueListSamplesPerSec);

	double fMinWithoutHrs = fMinTotal - (int)fHrsTotal * 60;
	double fSecWithoutMin = fSecTotal - (int)fMinTotal * 60;
	double fSecWithoutHrs = fSecTotal - (int)fHrsTotal * 3600;
	double fFrmWithoutSec = fFrmTotal - (int)fSecTotal * 75;
	double fFrmWithoutMin = fFrmTotal - (int)fMinTotal * 60 * 75;
	double fFrmWithoutHrs = fFrmTotal - (int)fHrsTotal * 3600 * 75;

	// Determine which placeholders exist
	strResult = "%m:%s";
	bool bHrsExist = (strResult.find("%h") != std::string::npos || strResult.find("%H") != std::string::npos);
	bool bMinExist = (strResult.find("%m") != std::string::npos || strResult.find("%M") != std::string::npos);
	bool bSecExist = (strResult.find("%s") != std::string::npos || strResult.find("%S") != std::string::npos);
	bool bFrmExist = (strResult.find("%f") != std::string::npos || strResult.find("%F") != std::string::npos);
	bool bSmpExist = (strResult.find("%p") != std::string::npos || strResult.find("%P") != std::string::npos);

	// Note the carry, which may be created by rounding up with sprintf ().
	// Then increase the higher-ranking position accordingly.
	bool bCarry = false;

	// Used to keep using sprintf format
	char tempFrames[100];
	char tempSeconds[100];
	char tempMinutes[100];
	char tempHours[100];
	char tempSamples[100];

	// Frames
	if (bFrmExist)
	{
		// Frames with seconds. Output number of decimal places
		if (bSecExist)
		{
			// Send two-digit frames minus seconds
			sprintf_s(tempFrames, sizeof(tempFrames), MakeFormatString(MFS_FRM, true).c_str(), fFrmWithoutSec);
			//strFrm.sprintf_s(MakeFormatString(MFS_FRM, true), fFrmWithoutSec);
			bCarry = CarryTimeString(strFrm, 75);
		}
		else if (bMinExist)
		{
			// Send two-digit frames minus minutes
			sprintf_s(tempFrames, sizeof(tempFrames), MakeFormatString(MFS_FRM, true).c_str(), fFrmWithoutMin);
			//strFrm.sprintf_s(MakeFormatString(MFS_FRM, true), fFrmWithoutMin);
			bCarry = CarryTimeString(strFrm, 60 * 75);
		}
		else if (bHrsExist)
		{
			// Send two-digit frames minus hours
			sprintf_s(tempFrames, sizeof(tempFrames), MakeFormatString(MFS_FRM, true).c_str(), fFrmWithoutHrs);
			//strFrm.sprintf_s(MakeFormatString(MFS_FRM, true), fFrmWithoutHrs);
			bCarry = CarryTimeString(strFrm, 60 * 60 * 75);
		}
		else
		{
			// Output leading zeros
			sprintf_s(tempFrames, sizeof(tempFrames), MakeFormatString(MFS_FRM, true).c_str(), fFrmTotal);
			//strFrm.sprintf_s(MakeFormatString(MFS_FRM, true), fFrmTotal);
		}
		strFrm = std::string(tempFrames);
	}

	// Seconds
	if (bSecExist)
	{
		// Consider carryover
		if (bCarry) fSecTotal++, fSecWithoutMin++, fSecWithoutHrs++, bCarry = false;

		if (bFrmExist)
		{
			// Send seconds without decimal places
			if (bMinExist)
			{
				// Send two digits minus the minutes
				sprintf_s(tempSeconds, sizeof(tempSeconds), MakeFormatString(MFS_SEC, false).c_str(), (int)fSecWithoutMin);
				//strSec.sprintf_s(MakeFormatString(MFS_SEC, false), (int)fSecWithoutMin);
				bCarry = CarryTimeString(strSec, 60);
			}
			else if (bHrsExist)
			{
				// Send two-digit seconds minus the hours
				sprintf_s(tempSeconds, sizeof(tempSeconds), MakeFormatString(MFS_SEC, false).c_str(), (int)fSecWithoutHrs);
				//strSec.sprintf_s(MakeFormatString(MFS_SEC, false), (int)fSecWithoutHrs);
				bCarry = CarryTimeString(strSec, 60 * 60);
			}
			else
			{
				// Output leading zeros
				sprintf_s(tempSeconds, sizeof(tempSeconds), MakeFormatString(MFS_SEC, false).c_str(), (int)fSecTotal);
				//strSec.sprintf_s(MakeFormatString(MFS_SEC, false), (int)fSecTotal);
			}
		}
		else
		{
			// Send seconds with decimal places
			if (bMinExist)
			{
				// Send two digits minus the minutes
				sprintf_s(tempSeconds, sizeof(tempSeconds), MakeFormatString(MFS_SEC, true).c_str(), fSecWithoutMin);
				//strSec.sprintf_s(MakeFormatString(MFS_SEC, true), fSecWithoutMin);
				bCarry = CarryTimeString(strSec, 60);
			}
			else if (bHrsExist)
			{
				// Send two-digit seconds minus the hours
				sprintf_s(tempSeconds, sizeof(tempSeconds), MakeFormatString(MFS_SEC, true).c_str(), fSecWithoutHrs);
				//strSec.sprintf_s(MakeFormatString(MFS_SEC, true), fSecWithoutHrs);
				bCarry = CarryTimeString(strSec, 60 * 60);
			}
			else
			{
				// Output number of leading zeros
				sprintf_s(tempSeconds, sizeof(tempSeconds), MakeFormatString(MFS_SEC, true).c_str(), fSecTotal);
				//strSec.sprintf_s(MakeFormatString(MFS_SEC, true), fSecTotal);
			}
		}
		strSec = std::string(tempSeconds);
	}

	// Minutes
	if (bMinExist)
	{
		// Consider carryover
		if (bCarry) fMinTotal++, fMinWithoutHrs++, bCarry = false;

		if (bSecExist || bFrmExist)
		{
			// Send minutes without decimal places
			if (bHrsExist)
			{
				// Send two-digit minutes minus hours
				sprintf_s(tempMinutes, sizeof(tempMinutes), MakeFormatString(MFS_SEC, false).c_str(), (int)fMinWithoutHrs);
				//strMin.sprintf_s(MakeFormatString(MFS_MIN, false), (int)fMinWithoutHrs);
				bCarry = CarryTimeString(strMin, 60);
			}
			else
			{
				// Output leading zeros
				sprintf_s(tempMinutes, sizeof(tempMinutes), MakeFormatString(MFS_SEC, false).c_str(), (int)fMinTotal);
				//strMin.sprintf_s(MakeFormatString(MFS_MIN, false), (int)fMinTotal);
			}
		}
		else
		{
			//Send minutes with decimal places
			if (bHrsExist)
			{
				// Send two-digit minutes minus hours
				sprintf_s(tempMinutes, sizeof(tempMinutes), MakeFormatString(MFS_MIN, true).c_str(), fMinWithoutHrs);
				//strMin.sprintf_s(MakeFormatString(MFS_MIN, true), fMinWithoutHrs);
				bCarry = CarryTimeString(strMin, 60);
			}
			else
			{
				// Output number of leading zeros
				sprintf_s(tempMinutes, sizeof(tempMinutes), MakeFormatString(MFS_MIN, true).c_str(), fMinTotal);
				//strMin.sprintf_s(MakeFormatString(MFS_MIN, true), fMinTotal);
			}
		}
		strMin = std::string(tempMinutes);
	}

	// Hours
	if (bHrsExist)
	{
		// Consider carryover
		if (bCarry) fHrsTotal++;     // (no further evaluation of the carry flag)

		if (bMinExist || bSecExist || bFrmExist)
		{
			// Hours with (?angeg bad translation). Number of leading zeros, but without decimal places
			sprintf_s(tempHours, sizeof(tempHours), MakeFormatString(MFS_HRS, false).c_str(), (int)fHrsTotal);
			//strHrs.sprintf_s(MakeFormatString(MFS_HRS, false), (int)fHrsTotal);
		}
		else
		{
			// Hours with (?angeg bad translation). Output number of leading zeros and decimal places
			sprintf_s(tempHours, sizeof(tempHours), MakeFormatString(MFS_HRS, true).c_str(), fHrsTotal);
			//strHrs.sprintf(MakeFormatString(MFS_HRS, true), fHrsTotal);
		}
		strHrs = std::string(tempHours);
	}

	// Samples
	if (bSmpExist)
	{
		// Samples with (?angeg bad translation). Output number of leading zeros
		sprintf_s(tempSamples, sizeof(tempSamples), MakeFormatString(MFS_SMP, true).c_str(), fSmpTotal);
		//strSmp.sprintf_s(MakeFormatString(MFS_SMP, true), fSmpTotal);
		strSmp = std::string(tempSamples);
	}

	// Replace wildcards with determined strings
	findAndReplaceAll(strResult, "%h", strHrs);	//strResult = StringReplace(strResult, "%h", strHrs, TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
	findAndReplaceAll(strResult, "%m", strMin);	//strResult = StringReplace(strResult, "%m", strMin, TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
	findAndReplaceAll(strResult, "%s", strSec);	//strResult = StringReplace(strResult, "%s", strSec, TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
	findAndReplaceAll(strResult, "%f", strFrm);	//strResult = StringReplace(strResult, "%f", strFrm, TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
	findAndReplaceAll(strResult, "%p", strSmp);	//strResult = StringReplace(strResult, "%p", strSmp, TReplaceFlags() << rfReplaceAll << rfIgnoreCase);

	return strResult;
}

//---------------------------------------------------------------------------

std::string MakeFormatString(int nFormatStringType, bool bFractional)
{
	char tempResult[100];
	int nLeadingDigits, nFractionalDigits;

	switch (nFormatStringType)
	{
	case MFS_HRS:
		nLeadingDigits = nFormatTimeLeadingDigitsHrs;
		nFractionalDigits = nFormatTimeFractionalDigitsHrs;
		break;
	case MFS_MIN:
		nLeadingDigits = nFormatTimeLeadingDigitsMin;
		nFractionalDigits = nFormatTimeFractionalDigitsMin;
		break;
	case MFS_SEC:
		nLeadingDigits = nFormatTimeLeadingDigitsSec;
		nFractionalDigits = nFormatTimeFractionalDigitsSec;
		break;
	case MFS_FRM:
		nLeadingDigits = nFormatTimeLeadingDigitsFrm;
		nFractionalDigits = nFormatTimeFractionalDigitsFrm;
		break;
	case MFS_SMP:
		nLeadingDigits = nFormatTimeLeadingDigitsSmp;
		nFractionalDigits = nFormatTimeFractionalDigitsSmp;
		break;
	}

	if (bFractional)
	{
		if (nFractionalDigits == 0)
		{
			sprintf_s(tempResult, sizeof(tempResult), "%%0%d.0Lf", nLeadingDigits);
			//strResult.sprintf("%%0%d.0Lf", nLeadingDigits);
		}
		else
		{
			sprintf_s(tempResult, sizeof(tempResult), "%%0%d.%dLf", nLeadingDigits + 1 + nFractionalDigits, nFractionalDigits);
			//strResult.sprintf("%%0%d.%dLf", nLeadingDigits + 1 + nFractionalDigits, nFractionalDigits);
		}
	}
	else
	{
		sprintf_s(tempResult, sizeof(tempResult), "%%0%dd", nLeadingDigits);
		//strResult.sprintf("%%0%dd", nLeadingDigits);
	}

	return std::string(tempResult);
}

//---------------------------------------------------------------------------

bool CarryTimeString(std::string strTime, int nMaxValue)
{
	int nPosDot = strTime.find_first_of(".");
	std::string strTimeIntValue = (nPosDot == std::string::npos ? strTime : strTime.substr(1, nPosDot - 1));
	std::string strTimeFracValue = (nPosDot == std::string::npos ? std::string("") : strTime.substr(nPosDot, strTime.length() - (nPosDot - 1)));
	int nTimeIntValue;
	try { nTimeIntValue = std::stoi(strTimeIntValue); }
	catch (...) { return false; }
	if (nTimeIntValue < nMaxValue) return false;
	strTimeIntValue = StringOfChar('0', strTimeIntValue.length());
	strTime = strTimeIntValue + strTimeFracValue;
	return true;
}

//---------------------------------------------------------------------------

std::string CueTypeToString(std::string strCueType)
{
	std::string strCueTypeString = "";

	if (strCueType == "rgn")
		strCueTypeString = "Basic";
	else if (strCueType == "beat")
		strCueTypeString = "Beat";
	else if (strCueType == "trak")
		strCueTypeString = "Track";
	else if (strCueType == "indx")
		strCueTypeString = "Index";

	return strCueTypeString;
}

//--------------------------------------------------------------------------- Custom File Name Extractor
std::string ExtractFileNameCustom(std::string strFileName)
{
	size_t posFileNameStart = strFileName.find_last_of("\\");
	size_t posFileNameEnd = strFileName.find_last_of(".wav");

	if (posFileNameStart == std::string::npos || posFileNameEnd == std::string::npos) 
	{
		OutputLogStr.append("Warning: Couldn't read file name...").append("\r\n");
		return "";
	}
	
	return strFileName.substr(posFileNameStart + 1, posFileNameEnd);
}

std::string GetCueInfo()
{
	std::ostringstream outputString;
	std::string strCue, strBegin, strEnd, strLength, strLabel, strNote, strFileName;
	strFileName = ExtractFileNameCustom(fileName);
	outputString << "File: " << strFileName << "\r\n";
	long float dBegin, dEnd, dDuration;
	for (int iCue = 0; iCue < nCues; iCue++)
	{
		dBegin = (CueArray[iCue].StartSample / nCueListSamplesPerSec);
		dEnd = ((CueArray[iCue].StartSample + CueArray[iCue].nSamples) / nCueListSamplesPerSec);
		dDuration = (CueArray[iCue].nSamples / nCueListSamplesPerSec);

		//OutputLogStr.append("- Begin time = ").append(std::to_string(dBegin)).append(" | End time = ").append(std::to_string(dEnd)).append("\r\n");

		strCue = std::to_string(iCue + 1);
		strBegin = FormatTime((long float)CueArray[iCue].StartSample / nCueListSamplesPerSec);
		strEnd = FormatTime((long float)(CueArray[iCue].StartSample + CueArray[iCue].nSamples) / nCueListSamplesPerSec);
		strLength = FormatTime((long float)CueArray[iCue].nSamples / nCueListSamplesPerSec);
		strLabel = CueArray[iCue].Label;
		strNote = CueArray[iCue].Description;
		//OutputLogStr.append("\t").append(std::to_string(GetTimeMillis(strBegin))).append(" | ").append(std::to_string(GetTimeMillis(strEnd))).append("\r\n");
		outputString << strCue << ") " << "Basic: " << strBegin << " - " << (CueArray[iCue].nSamples > 0 ? strEnd : "") << " " << strLabel << strNote << " (" << (CueArray[iCue].nSamples > 0 ? strLength : "") << ")\n";
	}

	return outputString.str();
}

void PrintCueInfo()
{
	std::ofstream txtOut;
	txtOut.open("C:\\Users\\matth\\OneDrive\\Town Scenes\\TestOutput.txt");

	std::ostringstream outputString;
	std::string strCue, strBegin, strEnd, strLength, strLabel, strNote, strFileName;
	strFileName = ExtractFileNameCustom(fileName);
	outputString << "File: " << strFileName << "\r\n";
	for (int iCue = 0; iCue < nCues; iCue++)
	{
		strCue = std::to_string(iCue + 1);
		strBegin = FormatTime((long float)CueArray[iCue].StartSample / nCueListSamplesPerSec);
		strEnd = FormatTime((long float) (CueArray[iCue].StartSample + CueArray[iCue].nSamples) / nCueListSamplesPerSec);
		strLength = FormatTime((long float) CueArray[iCue].nSamples / nCueListSamplesPerSec);
		strLabel = CueArray[iCue].Label;
		strNote = CueArray[iCue].Description;

		outputString << strCue << ") " << "Basic: " << strBegin << " - " << (CueArray[iCue].nSamples > 0 ? strEnd : "") << " " << strLabel << strNote << " (" << (CueArray[iCue].nSamples > 0 ? strLength : "") << ")\n";		
	}

	txtOut << outputString.str();
	txtOut.close();
}

#endif