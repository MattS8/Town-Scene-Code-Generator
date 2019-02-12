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
int CompareCueArrayElements(CUEARRAY* pElement1, CUEARRAY* pElement2);
void SwapCueArrayElements(CUEARRAY* pElement1, CUEARRAY* pElement2);

int nDataChunkSizeDiff;         // Difference in the size of the data chunks of the original
								// Wav file opposite the WavPack file
CUEARRAY* CueArray = new CUEARRAY[MAXCUES];
int nCues;                      // Number of existing cues
int nCueListSamplesPerSec;      // Samples per second of CueList

int nFilePointerCueSubchunk;    // Shows behind the wave data
int nMaxCues = MAXCUES;
std::string strCueSheetCDTitle;

// Information about the selected audio file

std::string strAudioFileName;
bool bAudioFileIsValid;
int nAudioFileSamples;
int nAudioFileSamplesPerSec;
WAVEFORMATEX WaveFmt;

// Variables replacing TOpenDialog
std::string fileName;


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
		int nBytesRead = fread(pChunk, 1, nBytesToRead, fp);
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

int CueSheetTimeStringToFrames(std::string strTimeString)
{
	int nFrames = -1;
	std::string strTemp(strTimeString);

	strTemp.replace(strTemp.begin(), strTemp.end(), ':', ' ');

	char *szMin = new char[strTemp.length() + 1];
	char *szSec = new char[strTemp.length() + 1];
	char *szFrm = new char[strTemp.length() + 1];

	int nAnzahl = sscanf_s(strTemp.c_str(), "%s %s %s", szMin, strTemp.length() + 1, szSec, strTemp.length() + 1, szFrm, strTemp.length() + 1);

	int nMin = 0, nSec = 0, nFrm = 0;

	do
	{
		if (nAnzahl >= 3)
		{
			try { nMin = std::stoi(szMin); }
			catch (...) { break; }
		}
		if (nAnzahl >= 2)
		{
			try { nSec = std::stoi(szSec); }
			catch (...) { break; }
		}
		if (nAnzahl >= 1)
		{
			try { nFrm = std::stoi(szFrm); }
			catch (...) { break; }
		}
		nFrames = nFrm + nSec * 75 + nMin * 60 * 75;
	} while (false);

	delete[] szMin;
	delete[] szSec;
	delete[] szFrm;

	return nFrames;
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

void LoadCueSheet(std::string strFileName)
{
	fileName = strFileName;
	/*if (strFileName == "") if (!OpenDialogCueSheet->Execute()) return;*/

	//// Should discard changes?
	//if (!DiscardChanges()) return;

	int nSampleRateCues = DEFAULT_SAMPLERATE;

	// Open cue sheet for reading

	FILE *fp;
	errno_t err = fopen_s(&fp, strFileName.c_str(), "rb");
	if (err != 0)
	{
		std::string strTemp = "Can't open Cue Sheet '"; strTemp.append(fileName).append("' for reading.");
		OutputLogStr.append(strTemp).append("/r/n");
		return;
	}

	// Cue-Sheet zeilenweise lesen und parsen

	std::string strLine, strToken, strTemp;
	std::string strCDFileName, strCDPerformer, strCDTitle;
	int iCue = 0, nLine = 0, nErrorLine = 0, nLastTrackCue = -1;
	bool bTrackExists = false, bFileExists = false;
	int nSamplesFile = 0, nSamplesRateFile = 0;
	bool bFileDataOK = false;
	unsigned int nOffsetSamples = 0;

	char szLine[256];
	while (fgets(szLine, sizeof(szLine), fp) != NULL)
	{
		nLine++;

		RemoveNewLine(szLine);
		strLine = szLine;
		strLine = trim(strLine);
		std::transform(strLine.begin(), strLine.end(), strLine.begin(), ::toupper);

		// TRACK: New Track
		strToken = "TRACK";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			if (bTrackExists)  // If tracks already exist, then index to the next track!
			{
				if (!CueExists(iCue)) iCue++;  // Avoid duplicates
			}

			if (!VerifyCueIndex(iCue))
			{
				iCue--;
				break;
			}

			// Neue Cue initialisieren
			CueArray[iCue].StartSample = 0;
			CueArray[iCue].nSamples = 0;
			CueArray[iCue].Label = "Track ";
			CueArray[iCue].Label.append(std::to_string(iCue+1));   // Track-Title
			CueArray[iCue].Description = strCDPerformer;                                 // Track-Performer
			CueArray[iCue].Type = "trak";

			bTrackExists = true;
			continue;
		}

		// PERFORMER: wenn noch kein TRACK, dann merken als "CD Performer", sonst merken als "Track Performer"
		strToken = "PERFORMER";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			strTemp = strLine.substr(strToken.length() + 1, strLine.length() - strToken.length());
			strTemp.replace(strLine.begin(), strLine.end(), ':', ' ');
			if (bTrackExists)
				CueArray[iCue].Description = strTemp;
			else
				strCDPerformer = strTemp;
			continue;
		}

		// TITLE: if no TRACK, then remember "CD Title", otherwise noted as "Track Title"
		strToken = "TITLE";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			strTemp = strLine.substr(strToken.length() + 1, strLine.length() - strToken.length());
			strTemp.replace(strLine.begin(), strLine.end(), '\"', ' ');
			if (bTrackExists)
				CueArray[iCue].Label = strTemp;
			else
				strCDTitle = strTemp;
			continue;
		}

		// FILE
		strToken = "FILE";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			int nPos1 = strLine.find("\"");
			if (nPos1 == 0 || nPos1 == strLine.length())
			{
				nErrorLine = nLine;
				break;
			}
			nPos1++;
			int nPos2 = strLine.substr(nPos1, strLine.length() - nPos1 + 1).find("\"");
			if (nPos2 == 0)
			{
				nErrorLine = nLine;
				break;
			}
			nPos2--;
			strTemp = strLine.substr(nPos1, nPos2);
			if (bFileExists)
			{
				if (bFileDataOK)
				{
					// Länge der letzten Datei zum Offset für die folgenden Zeitangaben addieren
					nOffsetSamples += ((unsigned int)nSamplesFile * nSampleRateCues) / nSamplesRateFile; //Unknown if changing hyper to unsigned int is correct...
					bFileDataOK = GetWavFileData(strTemp, nSamplesFile, nSamplesRateFile);
				}
				if (!bFileDataOK)
				{
					OutputLogStr.append("Cue Sheet must not contain more than 1 audio file, or all audio files must exist and be readable and valid!").append("\r\n");
					nErrorLine = -1;
					break;
				}
			}
			else
			{
				strCDFileName = strTemp;
				bFileExists = true;
				bFileDataOK = GetWavFileData(strTemp, nSamplesFile, nSamplesRateFile);
				if (bFileDataOK) nSampleRateCues = nSamplesRateFile;
			}
			continue;
		}

		// INDEX 01: Neue Track-Cue
		strToken = "INDEX 01";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			strTemp = trim(strLine.substr(strToken.length() + 1, strLine.length() - strToken.length()));
			int nFrames = CueSheetTimeStringToFrames(strTemp);
			if (nFrames == -1)
			{
				nErrorLine = nLine;
				break;
			}
			CueArray[iCue].StartSample = (int)(nOffsetSamples + (unsigned int)nFrames * nSampleRateCues / 75); // possible rounding error! //Unknown if changing hyper to unsigned int is correct...

			// Anzahl Samples des letzten Track-Indexes berechnen
			if (nLastTrackCue != -1) CueArray[nLastTrackCue].nSamples = CueArray[iCue].StartSample - CueArray[nLastTrackCue].StartSample;
			nLastTrackCue = iCue;
			continue;
		}

		// INDEX 00: Neue Index-Cue
		strToken = "INDEX 00";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			strTemp = trim(strLine.substr(strToken.length() + 1, strLine.length() - strToken.length()));
			int nFrames = CueSheetTimeStringToFrames(strTemp);
			if (nFrames == -1)
			{
				nErrorLine = nLine;
				break;
			}

			if (!CueExists(iCue)) iCue++;  // Avoid duplicates

			if (!VerifyCueIndex(iCue))
			{
				iCue--;
				break;
			}

			// Move the last cue back one to make room for the index cue
			CueArray[iCue].StartSample = CueArray[iCue - 1].StartSample;
			CueArray[iCue].nSamples = CueArray[iCue - 1].nSamples;
			CueArray[iCue].Label = CueArray[iCue - 1].Label;
			CueArray[iCue].Description = CueArray[iCue - 1].Description;
			CueArray[iCue].Type = CueArray[iCue - 1].Type;

			// Neue Cue initialisieren
			CueArray[iCue - 1].StartSample = (int)(nOffsetSamples + (unsigned int)nFrames * nSampleRateCues / 75); // possible rounding error! //Unknown if changing hyper to unsigned int is correct...
			CueArray[iCue - 1].nSamples = 0;
			CueArray[iCue - 1].Label = "Track index";
			CueArray[iCue - 1].Description = "";
			CueArray[iCue - 1].Type = "indx";
			continue;
		}

		// INDEX 02...n: Neue Index-Cue
		strToken = "INDEX";
		if (strLine.substr(1, strToken.length()).compare(strToken) == 0)
		{
			strTemp = trim(strLine.substr(strToken.length() + 3 + 1, strLine.length() - strToken.length() - 3));
			int nFrames = CueSheetTimeStringToFrames(strTemp);
			if (nFrames == -1)
			{
				nErrorLine = nLine;
				break;
			}

			if (!CueExists(iCue)) iCue++;  // Avoid duplicates

			if (!VerifyCueIndex(iCue))
			{
				iCue--;
				break;
			}

			// Neue Cue initialisieren
			CueArray[iCue].StartSample = (int)(nOffsetSamples + (unsigned int)nFrames * nSampleRateCues / 75); // possible rounding error! //Unknown if changing hyper to unsigned int is correct...
			CueArray[iCue].nSamples = 0;
			CueArray[iCue].Label = "Track index";
			CueArray[iCue].Description = "";
			CueArray[iCue].Type = "indx";
			continue;
		}
	}
	fclose(fp);

	if (nErrorLine == 0)
	{
		if (bTrackExists)
		{
			if (bFileDataOK)
			{
				// Add the length of the last file to the offset for the following times
				nOffsetSamples += ((unsigned int)nSamplesFile * nSampleRateCues) / nSamplesRateFile; //Unknown if changing hyper to unsigned int is correct...

				// Calculate the number of samples of the last track index
				// CueArray [iCue].nSamples = (int) nOffsetSamples - CueArray [iCue].StartSample; <- bug!
				if (nLastTrackCue != -1) CueArray[nLastTrackCue].nSamples = (int)nOffsetSamples - CueArray[nLastTrackCue].StartSample;

			}
			else
			{
				// Reset all sample lengths to 0, since without audio file the length of the last track
				// can not be calculated (if already track lengths, then for all tracks!)
				for (int i = 0; i <= iCue; i++)    // (iCue still points to the last cue in the array)
					CueArray[i].nSamples = 0;
			}

			// Correction: Index + 1 = number of cues
			if (!CueExists(iCue)) iCue++; // Avoid duplicates

			nCueListSamplesPerSec = nSampleRateCues;
		}
		//strCueSheetCDFileName = fileName;
		//SetCDPerformerAndTitleFromCDFileName();
		//if (!strCDFileName.IsEmpty()) strCueSheetCDFileName = strCDFileName;
		//if (!strCDPerformer.IsEmpty()) strCueSheetCDPerformer = strCDPerformer;
		strCueSheetCDTitle = !strCDTitle.empty() ? strCDTitle : "TODO_TITLE"; //TODO: generate title for file?
	}
	else
	{
		if (nErrorLine != -1)
		{
			OutputLogStr.append("Syntax error in line ").append(std::to_string(nErrorLine)).append(" of Cue Sheet\n'").append(fileName).append("'!").append("\r\n");
		}
		iCue = 0;
	}

	nCues = iCue;
	SortCueArray();
	OutputLogStr.append(std::to_string(nCues)).append(" Cues have been loaded from '").append(fileName).append("'").append("\r\n");
}

//---------------------------------------------------------------------------

void SelectAudioFile(std::string strFileName)
{
	if (strFileName != "")
		fileName = strFileName;
	else
		return;

	strAudioFileName = fileName;
	bAudioFileIsValid = GetWavFileData(strAudioFileName, nAudioFileSamples, nAudioFileSamplesPerSec);
}

//---------------------------------------------------------------------------  Custom String Formatter

std::string FormatTimeCustom(long float fSec)
{
	long float fHrsTotal = fSec / 3600;
	long float fMinTotal = fSec / 60;
	long float fSecTotal = fSec;

	int fMinWithoutHrs = round(fMinTotal - (int)fHrsTotal * 60);
	int fSecWithoutMin = round(fSecTotal - (int)fMinTotal * 60);
	int fMilisWithoutSec = round(((fSecTotal - (int)fMinTotal * 60) - fSecWithoutMin) * 1000);

	std::string strRet = "";

	return strRet.append(std::to_string(fMinWithoutHrs)).append(":")
		.append(fSecWithoutMin < 10 ? "0" : "").append(std::to_string(fSecWithoutMin)).append(".")
		.append(fMilisWithoutSec < 100 ? "0" : "").append(fMilisWithoutSec < 10 ? "0" : "").append(std::to_string(fMilisWithoutSec));
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
	for (int iCue = 0; iCue < nCues; iCue++)
	{
		strCue = std::to_string(iCue + 1);
		strBegin = FormatTimeCustom((long float)CueArray[iCue].StartSample / nCueListSamplesPerSec);
		strEnd = FormatTimeCustom((long float)(CueArray[iCue].StartSample + CueArray[iCue].nSamples) / nCueListSamplesPerSec);
		strLength = FormatTimeCustom((long float)CueArray[iCue].nSamples / nCueListSamplesPerSec);
		strLabel = CueArray[iCue].Label;
		strNote = CueArray[iCue].Description;

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
		strBegin = FormatTimeCustom((long float)CueArray[iCue].StartSample / nCueListSamplesPerSec);
		strEnd = FormatTimeCustom((long float) (CueArray[iCue].StartSample + CueArray[iCue].nSamples) / nCueListSamplesPerSec);
		strLength = FormatTimeCustom((long float) CueArray[iCue].nSamples / nCueListSamplesPerSec);
		strLabel = CueArray[iCue].Label;
		strNote = CueArray[iCue].Description;

		outputString << strCue << ") " << "Basic: " << strBegin << " - " << (CueArray[iCue].nSamples > 0 ? strEnd : "") << " " << strLabel << strNote << " (" << (CueArray[iCue].nSamples > 0 ? strLength : "") << ")\n";		
	}

	txtOut << outputString.str();
	txtOut.close();
}

#endif