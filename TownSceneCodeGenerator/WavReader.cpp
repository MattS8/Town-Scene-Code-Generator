#include "stdafx.h"
#include "WavReader.h"


// -------- WAV File Reader --------
// From CueListTool by S. B.

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
// Functions for sorting the cue array:

std::string CueArrayElementToSortString(CUEARRAY* pCueArrayElement)
{
	std::ostringstream ss;
	ss << pCueArrayElement->StartSample << pCueArrayElement->nSamples << pCueArrayElement->Label << pCueArrayElement->Description;
	ss << pCueArrayElement->Type << pCueArrayElement->StartSample << pCueArrayElement->nSamples << pCueArrayElement->Label;
	ss << pCueArrayElement->Description << pCueArrayElement->Type << "\r\n";

	return ss.str();
}