// TownSceneCodeGenerator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TownSceneCodeGenerator.h"
#include "WavReader.h"


#define _CRT_SECURE_NO_WARNINGS
#define SS_TRANSPARENT 0x00000100L
#define MAX_LOADSTRING 100

#define WRITE_TO_ARDUINO 2001
#define UPLOAD_CODE 2002
#define UPLOAD_TO_MP3 2003

#define USE_HALLOWEEN_CONTROLS 2004
#define CLEAR_LOG 2005
#define USE_LOW_PRECISION_TIMES 2006

#define PRETTY_PRINT 5
#define GENERATE_CODE 4
#define CREATE_ROUTINE 7
#define CLEAR_ROUTINES 8
#define ADD_DEBUG_STATEMENTS 9
#define USE_LIGHT 10
#define SWAP_ONOFF 3
#define SELECT_ALL 75
#define RANDOMIZE_ORDER 78
#define COPY_CODE 76
#define MOVE_ROUTINE_UP 148
#define MOVE_ROUTINE_UP_END (MOVE_ROUTINE_UP + MAX_NUM_ROUTINES)
#define MOVE_ROUTINE_DOWN (MOVE_ROUTINE_UP_END + 5)
#define MOVE_ROUTINE_DOWN_END (MOVE_ROUTINE_DOWN + MAX_NUM_ROUTINES)
#define DELETE_ROUTINE (MOVE_ROUTINE_DOWN_END + 5)
#define DELETE_ROUTINE_END (DELETE_ROUTINE + MAX_NUM_ROUTINES)
#define EDIT_ROUTINE (DELETE_ROUTINE_END + 5)
#define EDIT_ROUTINE_END (EDIT_ROUTINE + MAX_NUM_ROUTINES)
#define SAVE_EDIT_ROUTINE (EDIT_ROUTINE_END + 5)
#define SAVE_EDIT_ROUTINE_END (SAVE_EDIT_ROUTINE + MAX_NUM_ROUTINES)


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hwdHandler;

RoutineOrderTracker OrderTracker;
GenerationOptions Options;						// Code generation options

// UI Variables:
int RoutineHeight = 25;
int RoutineWidth = 100;
int RoutineBtnWidth = 100;
int RoutineBtnHeight = 25;
int width = 1300;
int height = 1300;
int RequiredFieldHeight = 150;

HWND gHWND;
HWND hMP3DriveLetter;
HWND hMP3Pin;
HWND hMotionSensorPin;
HWND hPrettyPrintBox;
HWND hDebugBox;
HWND hRandomizeRoutineOrder;
HWND hOutputLog;
HWND hMP3VolPin;
HWND hTrainLeftPin;
HWND hTrainRightPin;
HWND hTrainMotorPin;
HWND hSwapOnOffValues;
HWND hUseHalloweenMP3Controls;
HWND hAllLightsOnBlock;
HWND hTrainResetDuration;
HWND hClearLog;
HWND hUseLowPrecisionTimes;
HWND hRandomSeedPin;
HWND hRoutineScrollContainer; // Handle to scrollable container window
int totalRoutineHeight = 0; // Track total height of all routines


#define NUM_A_PINS 8
#define NUM_D_PINS 12
bool bchA[NUM_A_PINS] = { false };
unsigned int APinIds[NUM_A_PINS] = { IDM_LIGHTPINS_A0, IDM_LIGHTPINS_A1, IDM_LIGHTPINS_A2, IDM_LIGHTPINS_A3, IDM_LIGHTPINS_A4, IDM_LIGHTPINS_A5, IDM_LIGHTPINS_A6, IDM_LIGHTPINS_A7 };
bool bchD[NUM_D_PINS] = { false };
unsigned int DPinIds[NUM_D_PINS] = { IDM_LIGHTPINS_D2, IDM_LIGHTPINS_D3, IDM_LIGHTPINS_D4, IDM_LIGHTPINS_D5, IDM_LIGHTPINS_D6, IDM_LIGHTPINS_D7, IDM_LIGHTPINS_D8, IDM_LIGHTPINS_D9, IDM_LIGHTPINS_D10, IDM_LIGHTPINS_D11, IDM_LIGHTPINS_D12, IDM_LIGHTPINS_D13 };

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	AddRoutine(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	GenerateCodeCB(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK 	ScrollWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void				ParseRoutineInput(std::string input, std::string name, std::string fileName);
void				ParseWavFile(std::string name);
void				ParseFiles(HDROP wParam);
void				CheckSelectedPin(int wmId, HWND hWnd);

bool				RequiredFieldsFilled();
std::string			GetStringFromWindow(HWND hwnd);
void				AddControls(HWND  hwnd);
void				DrawRoutineList();
void				RemoveRoutineWindows();
std::string			GenerateCode();
bool				AllSelected(int set);
void				SelectAll(int set, bool setTo, HWND hWnd);
bool* GetBoolFromPinStr(std::string boolStr);
unsigned int GetUIDFromPinStr(std::string pinStr);

bool* GetBoolCB(unsigned int id);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TOWNSCENECODEGENERATOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TOWNSCENECODEGENERATOR));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//---------------------------------------------------------------------------

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TOWNSCENECODEGENERATOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TOWNSCENECODEGENERATOR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//---------------------------------------------------------------------------

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
   gHWND = hWnd;

   if (!hWnd)
   {
      return FALSE;
   }

   RECT rect;
   if (GetWindowRect(hWnd, &rect))
   {
	   width = rect.right - rect.left;
	   RoutineWidth = width / 2 - (RoutineBtnWidth*3) - 10;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//---------------------------------------------------------------------------

// Changes the order of the routines by moving one routine (based on the wmId) up or down
void MoveRoutine(int wmId, int direction)
{
	OutputLogStr.append("> Moving routine ").append(direction == UP ? "up" : "down").append("\r\n");
	SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
	// Get position of routine that should be moved
	int i = wmId - (direction == UP ? MOVE_ROUTINE_UP : MOVE_ROUTINE_DOWN);

	// If routine is alread at the top/bottom (respectively), do nothing
	int endOfList = direction == UP ? 0 : OrderTracker.routineNames.size() - 1;
	if (i == endOfList)
		return;

	// Swap the routine with its neighbor
	std::list<std::string>::iterator it, it2;
	std::string temp;
	it2 = OrderTracker.routineNames.begin();
	for (i; i > 0; ++it2, i--) ;
	temp = *it2;
	it = direction == UP ? it2-- : it2++;	
	*it = *it2;
	*it2 = temp;

	// Update the GUI
	DrawRoutineList();
}

//---------------------------------------------------------------------------

// Deletes the routine (based on the wmId) from the list
void DeleteRoutine(int wmId)
{
	// Remove all windows first so that the deleted routine's GUI is also deleted
	RemoveRoutineWindows();

	// Get position of routine to be deleted
	int i = wmId - DELETE_ROUTINE;

	// Find routine at position i
	std::list<std::string>::iterator it;
	for (it = OrderTracker.routineNames.begin(); i > 0; ++it, i--) ;

	// Erase from map and the name from OrderTracker
	Routines.erase(*it);
	OrderTracker.routineNames.erase(it);

	// Update the GUI
	DrawRoutineList();
}

bool CopySongToMp3Player(std::string mp3FilePath, std::string fileName)
{
	// Get Source File Path
	size_t mp3SourcePathSize = strlen(mp3FilePath.c_str()) + 1;
	wchar_t* wMp3SourcePath = new wchar_t[mp3SourcePathSize];
	size_t outSize;
	mbstowcs_s(&outSize, wMp3SourcePath, mp3SourcePathSize, mp3FilePath.c_str(), mp3SourcePathSize - 1);

	// Get Destination File Path
	std::string destPath = "";
	destPath.append(Options.mp3DriveLetter).append(":\\").append(fileName).append(".mp3");
	size_t mp3DestinationPathSize = strlen(destPath.c_str()) + 1;
	wchar_t* wMp3DestinationPath = new wchar_t[mp3DestinationPathSize];
	mbstowcs_s(&outSize, wMp3DestinationPath, mp3DestinationPathSize, destPath.c_str(), mp3DestinationPathSize - 1);

	// Attempt to Copy File to MP3
	bool bErrorFlag = CopyFile((LPCWSTR)wMp3SourcePath, (LPCWSTR) wMp3DestinationPath, false);
	delete[] wMp3SourcePath;
	if (bErrorFlag == FALSE) {
		std::ostringstream os;
		os << "> ---- Error: Unable to copy over " << fileName << " (Error Code: " << GetLastError() << ") ----\r\n";
		OutputLogStr.append(os.str());
		SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		return FALSE;
	}

	return TRUE;
}

void WriteCodeToArduino(bool uploadDirectly)
{
	std::string code;
	code = GenerateCode();

	SECURITY_ATTRIBUTES SA = { 0 };
	SA.nLength = sizeof(SECURITY_ATTRIBUTES);
	SA.bInheritHandle = TRUE;

	std::wstring wsTempDirectoryPath = L"C:\\temp\\TownSceneCodeGenerator";

	if (Options.mp3DriveLetter.size() == 0 && uploadDirectly) {
		OutputLogStr.append("> Warning: No MP3 drive letter was given. Files will not be uploaded to the MP3 player.\r\n");
	}

	std::list<std::string>::iterator it = OrderTracker.routineNames.begin();

	if (!Options.bUseHalloweenMP3Controls) {
		auto firstRoutineGUI = Routines.find(OrderTracker.routineNames.back());
		std::string firstMp3FilePath = firstRoutineGUI->second.routine.wavFilePath.substr(0, firstRoutineGUI->second.routine.wavFilePath.size() - 3);
		firstMp3FilePath.append("mp3");

		if (Options.mp3DriveLetter.size() > 0 && uploadDirectly) {
			bool firstSuccess = CopySongToMp3Player(firstMp3FilePath, firstRoutineGUI->second.routine.name);
			if (!firstSuccess)
				return;
		}
	}

	while (it != OrderTracker.routineNames.end()) {
		if (!Options.bUseHalloweenMP3Controls && std::next(it) == OrderTracker.routineNames.end())
			break;

		auto routineGUI = Routines.find(*it);
		std::string mp3FilePath = routineGUI->second.routine.wavFilePath.substr(0, routineGUI->second.routine.wavFilePath.size() - 3);
		mp3FilePath.append("mp3");
		//OutputLogStr.append("MP3 file path: ").append(mp3FilePath).append("\r\n");
		//SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());

	
		if (Options.mp3DriveLetter.size() > 0 && uploadDirectly) {
			bool success = CopySongToMp3Player(mp3FilePath, routineGUI->second.routine.name);
			if (!success)
				return;
		}
		++it;
	}

	// Create Directory
	std::wstring wsTempDir = L"C:\\temp";
	CreateDirectoryW(wsTempDir.c_str(), &SA);
	bool bErrorFlag = CreateDirectoryW(wsTempDirectoryPath.c_str(), &SA);

	//ERROR
	if (bErrorFlag == FALSE && GetLastError() != ERROR_ALREADY_EXISTS) {
		DWORD dwError = GetLastError();
		if (dwError == ERROR_PATH_NOT_FOUND) {
			OutputLogStr.append("> Error: Unable to create temporary directory for .ino file. (Path Not Found)\n");
			SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
			return;
		}
		else if (dwError != ERROR_ALREADY_EXISTS) {
			std::ostringstream os;
			os << "> ---- Error: Unable to create temporary directory for .ino file. (UNKNOWN ERROR: " << dwError << ") ----\r\n";
			OutputLogStr.append(os.str());
			SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
			return;
		}
	}

	DeleteFile(L"C:\\temp\\TownSceneCodeGenerator\\TownSceneCodeGenerator.ino");
	// Create File
	HANDLE hFileOut = ::CreateFile(L"C:\\temp\\TownSceneCodeGenerator\\TownSceneCodeGenerator.ino", GENERIC_WRITE,
		FILE_SHARE_WRITE,
		&SA,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// ERROR
	if (hFileOut == INVALID_HANDLE_VALUE) {
		OutputLogStr.append("> ---- Error: Unable to create temporary .ino file. ----\r\n");
		SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		return;
	}

	// Write to File
	const char* csCode = code.c_str();
	DWORD dwBytesWritten = 0;
	DWORD dwBytesToWrite = (DWORD)strlen(csCode);
	bErrorFlag = WriteFile(
		hFileOut,
		&csCode[0],
		dwBytesToWrite,
		&dwBytesWritten,
		NULL
	);
	::CloseHandle(hFileOut);

	//ERROR
	if (bErrorFlag == FALSE) {
		OutputLogStr.append("> ---- Error: Unable to write to temporary .ino file. ----\r\n");
		SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		return;
	}
	//ERROR
	else if (dwBytesToWrite != dwBytesWritten) {
		OutputLogStr.append("> ---- Error: Synchronous write to temporary .ino file failed. ----\r\n");
		SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		return;
	}
	//SUCCESS
	else {
		STARTUPINFO      SI = { 0 };
		PROCESS_INFORMATION PI = { 0 };
		SI.cb = sizeof(STARTUPINFO);
		SI.dwFlags = STARTF_USESTDHANDLES;
		SI.hStdOutput = hFileOut;

		const std::wstring command = uploadDirectly 
			? L"arduino_debug --upload C:\\temp\\TownSceneCodeGenerator\\TownSceneCodeGenerator.ino"
			: L"arduino C:\\temp\\TownSceneCodeGenerator\\TownSceneCodeGenerator.ino";

		::CreateProcess(0, (LPWSTR)command.c_str(), NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &SI, &PI);
		::CloseHandle(PI.hProcess);
		::CloseHandle(PI.hThread);

		if (uploadDirectly) {
			OutputLogStr.append("> ---- Sent code ");
			if (Options.mp3DriveLetter.size() > 0)
				OutputLogStr.append("and MP3s to devices!");
			else
				OutputLogStr.append("to device!");
			OutputLogStr.append(" ----\r\n");
		}
		else
			OutputLogStr.append("> ---- Successfully created .ino file at: 'C:\\temp\\TownSceneCodeGenerator\\TownSceneCodeGenerator.ino' ----\r\n");

		SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		::CloseHandle(PI.hThread);
		::CloseHandle(PI.hProcess);
		
	}
}

//---------------------------------------------------------------------------

// Commits the new routine name to the routine (based on the wmID)
void SaveNewRoutineName(int wmId)
{
	// Get position of routine

	// Find routine name at position i

	// Find routineGUI with corresponding name

	// Get new name from edit text box

	// Check new name for validity

	// Add pair to map with new name

	// Remove old occurance of routine from map

	// Change name in OrderTracker

	// Remove edit text window

	// Create static text window instead

	// Set static text window text to current name of routine

	// Change image of edit button to 'edit' icon


}

//---------------------------------------------------------------------------

// Allows the user to edit the name of a routine (based on the wmID)
void EditRoutine(int wmId)
{
	// Get position of routine to be editable

	// Find routine name at position i

	// Find routineGUI with corresponding name

	// Remove static text window

	// Create edit text window instead

	// Set edit text window text to current name of routine

	// Change image of edit button to 'done' checkmark
}

//---------------------------------------------------------------------------

// Add window procedure for scroll container:
LRESULT CALLBACK ScrollWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_COMMAND:
            // Forward command messages to parent window
            return SendMessage(GetParent(hWnd), message, wParam, lParam);
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hWnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
            return 1;
        }

        case WM_VSCROLL:
        {
            SCROLLINFO si = {0};
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);

            int yPos = si.nPos;
            switch (LOWORD(wParam))
            {
                case SB_LINEUP:
                    si.nPos -= 30;
                    break;
                case SB_LINEDOWN:
                    si.nPos += 30;
                    break;
                case SB_PAGEUP:
                    si.nPos -= si.nPage;
                    break;
                case SB_PAGEDOWN:
                    si.nPos += si.nPage;
                    break;
                case SB_THUMBTRACK:
                    si.nPos = si.nTrackPos;
                    break;
            }

            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            GetScrollInfo(hWnd, SB_VERT, &si);

            if (si.nPos != yPos)
            {
                ScrollWindow(hWnd, 0, yPos - si.nPos, NULL, NULL);
                UpdateWindow(hWnd);
            }
            return 0;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::map<std::string, Routine>::iterator it;
	std::map<std::string, HWND>::iterator itHWND;
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
			case IDM_GENERATEDCODEOPTIONS_PRETTYPRINT:
				Options.bPrettyPrint = !Options.bPrettyPrint;
				CheckMenuItem(GetMenu(hWnd), IDM_GENERATEDCODEOPTIONS_PRETTYPRINT, Options.bPrettyPrint ? MF_CHECKED : MF_UNCHECKED);
				break;
			case ID_DEBUG_TRAIN:
				Options.bDebugTrain = !Options.bDebugTrain;
				CheckMenuItem(GetMenu(hWnd), ID_DEBUG_TRAIN, Options.bDebugTrain ? MF_CHECKED : MF_UNCHECKED);
				break;
			case ID_DEBUG_LIGHTS:
				Options.bDebugLights = !Options.bDebugLights;
				CheckMenuItem(GetMenu(hWnd), ID_DEBUG_LIGHTS, Options.bDebugLights ? MF_CHECKED : MF_UNCHECKED);
				break;
			case ID_DEBUG_:
				Options.bDebugSkipRoutine = !Options.bDebugSkipRoutine;
				CheckMenuItem(GetMenu(hWnd), ID_DEBUG_, Options.bDebugSkipRoutine ? MF_CHECKED : MF_UNCHECKED);
				break;
			case IDM_GENERATEDCODEOPTIONS_DEBUGSTATEMENTS:
				Options.bAddDebugStatements = !Options.bAddDebugStatements;
				CheckMenuItem(GetMenu(hWnd), IDM_GENERATEDCODEOPTIONS_DEBUGSTATEMENTS, Options.bAddDebugStatements ? MF_CHECKED : MF_UNCHECKED);
				break;
			case IDM_GENERATEDCODEOPTIONS_RANDOMIZEROUTINEORDER:
				Options.bRandomizeRoutineOrder = !Options.bRandomizeRoutineOrder;
				CheckMenuItem(GetMenu(hWnd), IDM_GENERATEDCODEOPTIONS_RANDOMIZEROUTINEORDER, Options.bRandomizeRoutineOrder ? MF_CHECKED : MF_UNCHECKED);
				break;
			case IDM_GENERATEDCODEOPTIONS_SWAPONOFF:
				Options.bSwapOnOffValues = !Options.bSwapOnOffValues;
				CheckMenuItem(GetMenu(hWnd), IDM_GENERATEDCODEOPTIONS_SWAPONOFF, Options.bSwapOnOffValues ? MF_CHECKED : MF_UNCHECKED);
				return 0;
			case IDM_GENERATEDCODEOPTIONS_USELOWPRECISIONTIMES:
				Options.bUseLowPrecisionTimes = !Options.bUseLowPrecisionTimes;
				CheckMenuItem(GetMenu(hWnd), IDM_GENERATEDCODEOPTIONS_USELOWPRECISIONTIMES, Options.bUseLowPrecisionTimes ? MF_CHECKED : MF_UNCHECKED);
				break;
			case IDM_GENERATEDCODEOPTIONS_USEHALLOWEENMP3CONTROLS:
				Options.bUseHalloweenMP3Controls = !Options.bUseHalloweenMP3Controls;
				CheckMenuItem(GetMenu(hWnd), IDM_GENERATEDCODEOPTIONS_USEHALLOWEENMP3CONTROLS, Options.bUseHalloweenMP3Controls ? MF_CHECKED : MF_UNCHECKED);
				break;
			case IDM_LIGHTPINS_DSELUNSEL:
				SelectAll(1, !AllSelected(1), hWnd);
				break;
			case IDM_LIGHTPINS_ASELUNSEL:
				SelectAll(2, !AllSelected(2), hWnd);
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
			case IDM_INSTRTUCTIONS:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_VIEWGUIDE), hWnd, About);
				break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case PRETTY_PRINT:
				Options.bPrettyPrint = !Options.bPrettyPrint;
				CheckDlgButton(hWnd, PRETTY_PRINT, Options.bPrettyPrint ? BST_CHECKED : BST_UNCHECKED);
				break;
			case RANDOMIZE_ORDER:
				Options.bRandomizeRoutineOrder = !Options.bRandomizeRoutineOrder;
				CheckDlgButton(hWnd, RANDOMIZE_ORDER, Options.bRandomizeRoutineOrder ? BST_CHECKED : BST_UNCHECKED);
				break;
			case ADD_DEBUG_STATEMENTS:
				Options.bAddDebugStatements = !Options.bAddDebugStatements;
				CheckDlgButton(hWnd, ADD_DEBUG_STATEMENTS, Options.bAddDebugStatements ? BST_CHECKED : BST_UNCHECKED);
				break;
			case SWAP_ONOFF:
				Options.bSwapOnOffValues = !Options.bSwapOnOffValues;
				CheckDlgButton(hWnd, SWAP_ONOFF, Options.bSwapOnOffValues ? BST_CHECKED : BST_UNCHECKED);
				break;
			case USE_HALLOWEEN_CONTROLS:
				Options.bUseHalloweenMP3Controls = !Options.bUseHalloweenMP3Controls;
				CheckDlgButton(hWnd, USE_HALLOWEEN_CONTROLS, Options.bUseHalloweenMP3Controls ? BST_CHECKED : BST_UNCHECKED);
				break;
			case CREATE_ROUTINE:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDROUTINE), hWnd, AddRoutine);
				break;
			case CLEAR_ROUTINES:
				RemoveRoutineWindows();
				OutputLogStr.append("> Cleared all routines from list.\r\n");
				SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
				Routines = std::map<std::string, RoutineGUI>();
				OrderTracker.routineNames.clear();
				break;
			case GENERATE_CODE:
				Options.mp3SkipPin = GetStringFromWindow(hMP3Pin);
				Options.motionSensorPin = GetStringFromWindow(hMotionSensorPin);
				Options.mp3VolumePin = GetStringFromWindow(hMP3VolPin);
				Options.trainPinLeft = GetStringFromWindow(hTrainLeftPin);
				Options.trainPinRight = GetStringFromWindow(hTrainRightPin);
				Options.motorVoltagePin = GetStringFromWindow(hTrainMotorPin);
				Options.mp3DriveLetter = GetStringFromWindow(hMP3DriveLetter);
				//Options.allLightsOnBlock = GetLongFromWindow(hAllLightsOnBlock);
				Options.trainResetDuration = GetLongFromWindow(hTrainResetDuration);
				Options.randomSeedPin = GetStringFromWindow(hRandomSeedPin);
				if (RequiredFieldsFilled())
					DialogBox(hInst, MAKEINTRESOURCE(IDD_VIEWCODE), hWnd, GenerateCodeCB);
				else
					SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
				break;
			case WRITE_TO_ARDUINO:
				Options.mp3SkipPin = GetStringFromWindow(hMP3Pin);
				Options.motionSensorPin = GetStringFromWindow(hMotionSensorPin);
				Options.mp3VolumePin = GetStringFromWindow(hMP3VolPin);
				Options.trainPinLeft = GetStringFromWindow(hTrainLeftPin);
				Options.trainPinRight = GetStringFromWindow(hTrainRightPin);
				Options.motorVoltagePin = GetStringFromWindow(hTrainMotorPin);
				Options.mp3DriveLetter = GetStringFromWindow(hMP3DriveLetter);
				//Options.allLightsOnBlock = GetLongFromWindow(hAllLightsOnBlock);
				Options.trainResetDuration = GetLongFromWindow(hTrainResetDuration);
				Options.randomSeedPin = GetStringFromWindow(hRandomSeedPin);
				if (RequiredFieldsFilled())
					WriteCodeToArduino(FALSE);
				else
					SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
				break;
			case UPLOAD_CODE:
				Options.mp3SkipPin = GetStringFromWindow(hMP3Pin);
				Options.motionSensorPin = GetStringFromWindow(hMotionSensorPin);
				Options.mp3VolumePin = GetStringFromWindow(hMP3VolPin);
				Options.trainPinLeft = GetStringFromWindow(hTrainLeftPin);
				Options.trainPinRight = GetStringFromWindow(hTrainRightPin);
				Options.motorVoltagePin = GetStringFromWindow(hTrainMotorPin);
				Options.mp3DriveLetter = GetStringFromWindow(hMP3DriveLetter);
				//Options.allLightsOnBlock = GetLongFromWindow(hAllLightsOnBlock);
				Options.trainResetDuration = GetLongFromWindow(hTrainResetDuration);
				Options.randomSeedPin = GetStringFromWindow(hRandomSeedPin);
				if (RequiredFieldsFilled())
					WriteCodeToArduino(TRUE);
				else
					SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
				break;
			case UPLOAD_TO_MP3:
				Options.bUploadToMp3 = !Options.bUploadToMp3;
				
				break;
			case CLEAR_LOG:
				OutputLogStr = "";
				SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
				break;
			case USE_LOW_PRECISION_TIMES:
				Options.bUseLowPrecisionTimes = !Options.bUseLowPrecisionTimes;
				CheckDlgButton(hWnd, wmId, Options.bUseLowPrecisionTimes ? BST_CHECKED : BST_UNCHECKED);
			case ID_LIGHTPINS_USELEGACY:
				Options.bUseLegacyA6 = !Options.bUseLegacyA6;
				CheckDlgButton(hWnd, wmId, Options.bUseLegacyA6 ? BST_CHECKED : BST_UNCHECKED);
				break;
			case ID_LIGHTPINS_USELEGACYA7:
				Options.bUseLegacyA7 = !Options.bUseLegacyA7;
				CheckDlgButton(hWnd, wmId, Options.bUseLegacyA7 ? BST_CHECKED : BST_UNCHECKED);
				break;
            default:
				if (wmId == IDM_LIGHTPINS_D2 || wmId == IDM_LIGHTPINS_D3 || wmId == IDM_LIGHTPINS_D4 || wmId == IDM_LIGHTPINS_D5
					|| wmId == IDM_LIGHTPINS_D6 || wmId == IDM_LIGHTPINS_D7 || wmId == IDM_LIGHTPINS_D8 || wmId == IDM_LIGHTPINS_D9
					|| wmId == IDM_LIGHTPINS_D10 || wmId == IDM_LIGHTPINS_D11 || wmId == IDM_LIGHTPINS_D12 || wmId == IDM_LIGHTPINS_D13
					|| wmId == IDM_LIGHTPINS_A0 || wmId == IDM_LIGHTPINS_A1 || wmId == IDM_LIGHTPINS_A2 || wmId == IDM_LIGHTPINS_A3 
					|| wmId == IDM_LIGHTPINS_A4 || wmId == IDM_LIGHTPINS_A5 || wmId == IDM_LIGHTPINS_A6 || wmId == IDM_LIGHTPINS_A7)
				{
					bool* bCb = GetBoolCB(wmId);
					*bCb = !*bCb;
					CheckMenuItem(GetMenu(hWnd), wmId, *bCb ? MF_CHECKED : MF_UNCHECKED);
					break;
				}
				else if		(wmId >= USE_LIGHT && wmId <= USE_LIGHT + 18)					CheckSelectedPin(wmId, hWnd);
				else if (wmId >= MOVE_ROUTINE_DOWN && wmId <= MOVE_ROUTINE_DOWN_END)	MoveRoutine(wmId, DOWN);
				else if (wmId >= MOVE_ROUTINE_UP && wmId <= MOVE_ROUTINE_UP_END)		MoveRoutine(wmId, UP);
				else if (wmId >= DELETE_ROUTINE && wmId <= DELETE_ROUTINE_END)			DeleteRoutine(wmId);
				else if (wmId >= EDIT_ROUTINE && wmId <= EDIT_ROUTINE_END)				EditRoutine(wmId);
				else if (wmId >= SAVE_EDIT_ROUTINE && wmId <= SAVE_EDIT_ROUTINE_END)	SaveNewRoutineName(wmId);
				else
					return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
	case WM_CREATE:
		AddControls(hWnd);
		DragAcceptFiles(hWnd, true);
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_DROPFILES:
		ParseFiles((HDROP) wParam);
		break;
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
		return 1;
	}
	// case WM_VSCROLL:
    //     {
    //         SCROLLINFO si = {0};
    //         si.cbSize = sizeof(SCROLLINFO);
    //         si.fMask = SIF_ALL;
    //         GetScrollInfo(hWnd, SB_VERT, &si);

    //         int yPos = si.nPos;
    //         switch (LOWORD(wParam))
    //         {
    //             case SB_LINEUP:
    //                 si.nPos -= 30;
    //                 break;
    //             case SB_LINEDOWN:
    //                 si.nPos += 30;
    //                 break;
    //             case SB_PAGEUP:
    //                 si.nPos -= si.nPage;
    //                 break;
    //             case SB_PAGEDOWN:
    //                 si.nPos += si.nPage;
    //                 break;
    //             case SB_THUMBTRACK:
    //                 si.nPos = si.nTrackPos;
    //                 break;
    //         }

    //         si.fMask = SIF_POS;
    //         SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    //         GetScrollInfo(hWnd, SB_VERT, &si);

    //         if (si.nPos != yPos)
    //         {
    //             ScrollWindow(hWnd, 0, yPos - si.nPos, NULL, NULL);
    //             UpdateWindow(hWnd);
    //         }
    //         break;
    //     }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//---------------------------------------------------------------------------

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//---------------------------------------------------------------------------

// Message handler for generated code box.
INT_PTR CALLBACK GenerateCodeCB(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::string code;
	//UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		code = GenerateCode();
		SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT1), std::wstring(code.begin(), code.end()).c_str());
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCOPYCODE)
		{
			// Copy code to clipboard
			code = GetStringFromWindow(GetDlgItem(hDlg, IDC_EDIT1));
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, code.size() + 1);
			memcpy(GlobalLock(hMem), code.c_str(), code.size() + 1);
			GlobalUnlock(hMem);
			OpenClipboard(0);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, hMem);
			CloseClipboard();
			return (INT_PTR)TRUE;
		}
		break;
	}

	return (INT_PTR) FALSE;
}

//---------------------------------------------------------------------------

// Message handler for Add Routine box.
INT_PTR CALLBACK AddRoutine(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::vector<wchar_t> buf;
	std::string input, name;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDOK)
		{
			int len = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_EDIT1)) + 1;

			if (len > 0)
			{
				// Check for custom name input
				buf = std::vector<wchar_t>(len);
				GetDlgItemTextW(hDlg, IDC_EDIT1, &buf[0], len);
				input = std::string(buf.begin(), buf.end());

				len = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_EDIT2)) + 1;
				if (len > 1)
				{
					// Set routine name to cutom name
					buf = std::vector<wchar_t>(len);
					GetDlgItemTextW(hDlg, IDC_EDIT2, &buf[0], len);
					name = std::string(buf.begin(), buf.end());
					name.pop_back();

					// Check for valid variable name
					if (!IsValidVarName(name))
					{
						OutputLogStr.append("> Warning: Filename '").append(name).append("' cannot be used as a variable name. Renaming to '");
						name = GenerateRoutineName();
						OutputLogStr.append(name).append("' instead.").append("\r\n");
					}
				}
				else {
					name = GenerateRoutineName();
				}

				// Get routine from cue sheet data
				ParseRoutineInput(input, name, "");

				// Update GUI
				DrawRoutineList();
				SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
			}
			else {
				OutputLogStr.append("> Warning: No input found. Make sure you pasted the routine data into the dialog box that appears when you click \"Add Routine\"\r\n");
				SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
			}

			// Close dialog after done
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

//---------------------------------------------------------------------------

// Determines if user has entered enough data to generate the Arduino code
bool RequiredFieldsFilled()
{
	bool bRequiredFieldsFilled = true;
	
	// Check if random seed set, but not using random routines
	if (!Options.bRandomizeRoutineOrder && !Options.randomSeedPin.empty())
	{
		OutputLogStr.append("> ---- Warning: A pin was declared for random seed initialization, but the option to randomize routines was not enabled! ----\r\n");
	}

	// Need at least one routine
	if (Routines.size() == 0) 
	{
		OutputLogStr.append("> ---- Error: Nothing to show. Please add routines and try again. ----\r\n");
		//SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		bRequiredFieldsFilled = false;
	}

	// Need pin number of the MP3 player
	if (Options.mp3SkipPin.size() == 0)
	{
		OutputLogStr.append("> ---- Error: No pins declared for the MP3 Player. ----\r\n");
		//SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
		bRequiredFieldsFilled = false;
	}

	// Need train reset duration if train pin is declared
	// Update 2.0 - Train Reset Duration can be left empty in which case an average will be used based on motor readings
	
	//if ((!Options.trainPinLeft.empty() || !Options.trainPinRight.empty()) && Options.trainResetDuration == 0) {
	//	OutputLogStr.append("> ---- Error: Train Reset Duration must be set if a train pin is declared. ----\r\n");
	//	//SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
	//	bRequiredFieldsFilled = false;
	//}

	// Need pin number for the volume controls

	return bRequiredFieldsFilled;
}

//---------------------------------------------------------------------------

HFONT g_HeaderFont = NULL;
HFONT g_ButtonFont = NULL;
HFONT g_OptionsHeaderFont = NULL;

// Sets up the GUI for all the options and controls of this application
void AddControls(HWND handler)
{
	if (g_HeaderFont == NULL)
	{
		g_HeaderFont = CreateFont(20, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	}

	if (g_ButtonFont == NULL)
	{
		g_ButtonFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	}
	if (g_OptionsHeaderFont == NULL)
	{
		g_OptionsHeaderFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	}

	RECT rect;
	int i;
	if (GetWindowRect(handler, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		RoutineWidth = width / 2 - (RoutineBtnWidth * 3) - 50;
	}

	hwdHandler = handler;

	int HeaderRowHeights[4];
	int OptionsRowHeight[7];

	HeaderRowHeights[0] = 7;
	OptionsRowHeight[0] = HeaderRowHeights[0] + 20;
	OptionsRowHeight[1] = OptionsRowHeight[0];
	OptionsRowHeight[2] = OptionsRowHeight[1];
	HeaderRowHeights[1] = OptionsRowHeight[2] + 40;
	OptionsRowHeight[3] = HeaderRowHeights[1] + 25;
	HeaderRowHeights[2] = OptionsRowHeight[3] + 40;
	OptionsRowHeight[4] = HeaderRowHeights[2] + 25;
	OptionsRowHeight[5] = OptionsRowHeight[4] + 25;
	HeaderRowHeights[3] = OptionsRowHeight[5] + 55;
	OptionsRowHeight[6] = HeaderRowHeights[3] + 25;


	int ColumSpace = 10;
	int ItemHeight = 25;
	int HeaderHeight = 20;
	int LabelHeight = 20;
	int InputHeight = 20;

	int NextItemW = 0;
	int NextItemH = 0;


	int cbHeight = 20;
	int secondColumnStart = RoutineWidth + (RoutineBtnWidth*3) + 75;
	int reqFielsLen = 105;
	int pinEditWidth = 35;
	int thirdColumnStart = secondColumnStart + reqFielsLen + pinEditWidth + 60;
	int directUploadOptionsYPos = 150 + 55 + 70;

    // Register scroll container window class
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = ScrollWndProc;  // Use your existing ScrollWndProc
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.lpszClassName = L"SCROLLWIN";
    RegisterClassExW(&wcex);

	// Create Routine scrollable container window
	hRoutineScrollContainer = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"SCROLLWIN",  // Use the same class name as registered
		L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
		5,
		HeaderRowHeights[0] + 15 + 60,
		RoutineWidth + (RoutineBtnWidth * 3) + 30,
		height - 175,
		handler,
		NULL,
		NULL,
		NULL
	);

	// Routines
	NextItemH = HeaderRowHeights[0];
	HWND hStatic = CreateWindowW(L"Static", L"ROUTINES", WS_VISIBLE | WS_CHILD, 5, HeaderRowHeights[0], secondColumnStart - 30, 45 + 20, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_HeaderFont, TRUE);
	hStatic = CreateWindowW(L"Button", L"Add", WS_VISIBLE | WS_CHILD, 5, 35, 120, 30, handler, (HMENU)CREATE_ROUTINE, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	hStatic = CreateWindowW(L"Button", L"Clear All", WS_VISIBLE | WS_CHILD, 5 + 130, 35, 140, 30, handler, (HMENU)CLEAR_ROUTINES, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);

	// Train Options
	NextItemW = secondColumnStart;
	hStatic = CreateWindowW(L"Static", L"OPTIONS", WS_VISIBLE | WS_CHILD, NextItemW, HeaderRowHeights[0], secondColumnStart - 30, 45 + 20, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_HeaderFont, TRUE);
	NextItemH += 60;
	//CreateWindowW(L"Static", L"TRAIN OPTIONS", WS_VISIBLE | WS_CHILD, secondColumnStart, HeaderRowHeights[0], 120, 20, handler, NULL, NULL, NULL);
	CreateWindowW(L"Static", L"Train Init Duration: ", WS_VISIBLE | WS_CHILD | SS_TRANSPARENT, NextItemW, NextItemH, 125, LabelHeight, handler, NULL, NULL, NULL);
	NextItemW += 125;
	hTrainResetDuration = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, 55, InputHeight, handler, NULL, NULL, NULL);
	NextItemW += 55 + ColumSpace;
	CreateWindowW(L"Static", L"Train Pin (Left to Right): ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 175, LabelHeight, handler, NULL, NULL, NULL);
	NextItemW += 155;
	hTrainLeftPin = CreateWindowW(L"Edit", L"A5", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, LabelHeight, handler, NULL, NULL, NULL);
	NextItemW += pinEditWidth + ColumSpace;
	CreateWindowW(L"Static", L"Train Pin (Right to Left): ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 175, LabelHeight, handler, NULL, NULL, NULL);
	NextItemW += 155;
	hTrainRightPin = CreateWindowW(L"Edit", L"A4", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, LabelHeight, handler, NULL, NULL, NULL);
	NextItemW += pinEditWidth;
	int HeaderCenter = secondColumnStart;/*secondColumnStart + (NextItemW - secondColumnStart - 160) / 2;*/
	hStatic = CreateWindowW(L"Static", L"TRAIN OPTIONS:", WS_VISIBLE | WS_CHILD, HeaderCenter, NextItemH-25, 160, 20, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);

	NextItemW = secondColumnStart;
	NextItemH += 25;
	CreateWindowW(L"Static", L"Motor Voltage Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 125, LabelHeight, handler, NULL, NULL, NULL);
	NextItemW += 125;
	hTrainMotorPin = CreateWindowW(L"Edit", L"A6", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, LabelHeight, handler, NULL, NULL, NULL);
	
	// MP3 Options
	NextItemW = secondColumnStart;
	NextItemH += 35+30;
	CreateWindowW(L"Static", L"Power/Skip Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 105, 20, handler, NULL, NULL, NULL);
	NextItemW += 105;
	hMP3Pin = CreateWindowW(L"Edit", L"A3", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	NextItemW += pinEditWidth + ColumSpace;
	CreateWindowW(L"Static", L"Volume Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 83, 20, handler, NULL, NULL, NULL);
	NextItemW += 83;
	hMP3VolPin = CreateWindowW(L"Edit", L"A2", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	NextItemW += pinEditWidth + ColumSpace;
	CreateWindowW(L"Static", L"Windows Drive Letter: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 150, 20, handler, NULL, NULL, NULL);
	NextItemW += 150;
	hMP3DriveLetter = CreateWindowW(L"Edit", L"D", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	NextItemW += pinEditWidth;
	hStatic = CreateWindowW(L"Static", L"MP3 OPTIONS:", WS_VISIBLE | WS_CHILD, HeaderCenter, NextItemH-20, 155, 20, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);

	// Misc Options
	NextItemW = secondColumnStart;
	NextItemH += 35 + 30;
	hStatic = CreateWindowW(L"Static", L"MISC OPTIONS:", WS_VISIBLE | WS_CHILD, HeaderCenter, NextItemH - 20, 155, 20, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	//CreateWindowW(L"Static", L"\"All Lights On\" Init Duration: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 185, 20, handler, NULL, NULL, NULL);
	//NextItemW += 185;
	//hAllLightsOnBlock = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, 55, InputHeight, handler, NULL, NULL, NULL);
	//NextItemW += 55 + ColumSpace;
	CreateWindowW(L"Static", L"Motion Sense Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 122, 20, handler, NULL, NULL, NULL);
	NextItemW += 122;
	hMotionSensorPin = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	NextItemW += pinEditWidth + ColumSpace;
	CreateWindowW(L"Static", L"Randomize Seed Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 145, 20, handler, NULL, NULL, NULL);
	NextItemW += 145;
	hRandomSeedPin = CreateWindowW(L"Edit", L"A7", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);

	// Bottom Bar
	int bottomStart = OptionsRowHeight[4] + (cbHeight * 8) + 25;
	int bottomColumStart = secondColumnStart + ((thirdColumnStart - 200 - secondColumnStart) / 2);
	int outputLength = 0;
	hStatic = CreateWindowW(L"Button", L"View Generated Code", WS_VISIBLE | WS_CHILD, 
		bottomColumStart,
		bottomStart, 
		200, 
		35, 
		handler, (HMENU)GENERATE_CODE, NULL, NULL);
		SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	outputLength += 200;
	hStatic = CreateWindowW(L"Button", L"Create .ino File", WS_VISIBLE | WS_CHILD, 
		bottomColumStart + 210,
		bottomStart, 
		200, 
		35, 
		handler, (HMENU)WRITE_TO_ARDUINO, NULL, NULL);
		SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	outputLength += 210;
	hStatic = CreateWindowW(L"Button", L"Upload to Device(s)", WS_VISIBLE | WS_CHILD, 
		bottomColumStart + 210 + 210,
		bottomStart, 
		150, 35, 
		handler, (HMENU)UPLOAD_CODE, NULL, NULL);
		SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
		
	outputLength += 185;
	hStatic = CreateWindowW(L"Static", L"Output Log: ", WS_VISIBLE | WS_CHILD, 
		bottomColumStart,
		bottomStart + 45, 
		950,
		20, 
		handler, NULL, NULL, NULL);
		SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	hClearLog = CreateWindowW(L"Button", L"Clear  (X)", WS_VISIBLE | WS_CHILD, 
		bottomColumStart + 100,
		bottomStart + 45,
		99,
		20,
		handler, (HMENU)CLEAR_LOG, NULL, NULL);
		SendMessage(hClearLog, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	hOutputLog = CreateWindowW(L"Edit", L"", 
		WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | 
		ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // Keep these flags
		bottomColumStart,
		bottomStart + 65, 
		outputLength, 
		height - (bottomStart + 150),
		handler, NULL, NULL, NULL);
	
}

//---------------------------------------------------------------------------

// Removes all GUI windows for the list of routines. Useful for when the order has changed.
void RemoveRoutineWindows()
{
	std::map<std::string, RoutineGUI>::iterator it;
	for (it = Routines.begin(); it != Routines.end(); ++it)
	{
		if (it->second.title != NULL) DestroyWindow(it->second.title);
		if (it->second.downButton != NULL) DestroyWindow(it->second.downButton);
		if (it->second.upButton != NULL) DestroyWindow(it->second.upButton);
		if (it->second.deleteButton != NULL) DestroyWindow(it->second.deleteButton);
		if (it->second.editButton != NULL) DestroyWindow(it->second.editButton);
		it->second.title = NULL;
		it->second.downButton = NULL;
		it->second.upButton = NULL;
		it->second.deleteButton = NULL;
		it->second.editButton = NULL;
	}
}

//---------------------------------------------------------------------------

// Updates the GUI with a list of routines in the propper order
void DrawRoutineList()
{
	// Remove current GUI objects
	RemoveRoutineWindows();

	int i = 0;
	int buttonPos = 0;
	std::list<std::string>::iterator itter;
	RoutineGUI* routineGUI;
	int currentYPos = 5;
	totalRoutineHeight = 1;
	for (itter = OrderTracker.routineNames.begin(); itter != OrderTracker.routineNames.end(); ++itter, buttonPos = 0)
	{
		// Get routine corresponding to this current position in the list
		routineGUI = &(Routines.find(*itter)->second);

		// Create string containing the routine name and how many lights are in this routine
		std::ostringstream osRoutineStr;
		osRoutineStr << ++i << ") " << routineGUI->routine.name << " (" << routineGUI->routine.lights.size() << " lights)\r\n";
		std::string tempStr = osRoutineStr.str();
		std::wstring wStr = std::wstring(tempStr.begin(), tempStr.end());

		// Create windows for the routine name, and buttons to change the routine's position in the list
		int rowHeight = (wStr.length() > 50 ? RoutineHeight * 2 : RoutineHeight);
		int buttonYPos = (wStr.length() > 50 ? currentYPos-(RoutineHeight) : currentYPos);
		routineGUI->title = CreateWindowW(L"Static", wStr.c_str(), WS_VISIBLE | WS_CHILD, 
			5, 
			currentYPos, 
			RoutineWidth, 
			rowHeight,
			hRoutineScrollContainer, NULL, NULL, NULL);
		routineGUI->upButton = CreateWindowW(L"Button", L"Move Up", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + 10 + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth, 
			RoutineBtnHeight, 
			hRoutineScrollContainer, (HMENU)(MOVE_ROUTINE_UP + i - 1), NULL, NULL);
		routineGUI->downButton = CreateWindowW(L"Button", L"Move Down", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + 10 + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth, RoutineBtnHeight, 
			hRoutineScrollContainer, (HMENU)(MOVE_ROUTINE_DOWN + i - 1), NULL, NULL);
		//routineGUI->editButton = CreateWindowW(L"Button", L"e", WS_VISIBLE | WS_CHILD, RoutineWidth + 10 + (RoutineBtnWidth * buttonPos++), 45 + (RoutineHeight * i) + (i * 5), RoutineBtnWidth, RoutineBtnHeight, hwdHandler, (HMENU)(EDIT_ROUTINE + i - 1), NULL, NULL);
		routineGUI->deleteButton = CreateWindowW(L"Button", L"X", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + 10 + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth/2, 
			RoutineBtnHeight,
			hRoutineScrollContainer, (HMENU)(DELETE_ROUTINE + i - 1), NULL, NULL);

		totalRoutineHeight += 5 + rowHeight;
		currentYPos += 5 + rowHeight;

		//auto hwndEdit = LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		//SendMessage(routineGUI->editButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hwndEdit);
		//auto hwndDown = LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP3), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		//SendMessage(routineGUI->downButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hwndDown);
		//auto hwndUp = LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP4), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		//SendMessage(routineGUI->upButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hwndUp);
		//auto hwndDelete = LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		//SendMessage(routineGUI->upButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hwndUp);

	}

	totalRoutineHeight += 15;

	// Set scroll info
	SCROLLINFO si = {0};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMin = 0;
	si.nMax = totalRoutineHeight;
	si.nPage = height - 175; // Visible height of scroll container
	SetScrollInfo(hRoutineScrollContainer, SB_VERT, &si, TRUE);

}

//---------------------------------------------------------------------------

// Read files that were dropped onto application
void ParseFiles(HDROP hDropInfo)
{
	unsigned int numFiles = DragQueryFileW(hDropInfo, 0xFFFFFFFF, NULL, NULL);
	unsigned int filenameSize;
	char buff[500] = { 0 };
	char* pch;

	// Check for any readable files
	if (numFiles > 0)
	{
		for (int i = 0; i < numFiles; ++i)
		{
			filenameSize = DragQueryFileW(hDropInfo, i, NULL, NULL);
			DragQueryFileA(hDropInfo, i, buff, filenameSize + 1);
			pch = std::strstr((char*) buff, ".wav");

			// Check if file is a .wav file
			if (pch != NULL) {
				ParseWavFile(std::string(buff));
			}
			else {
				OutputLogStr.append("> ---- Error: Not a .wav file.").append("\r\n").append(std::string((char*) buff)).append("\r\n").append("----\r\n");
			}
		}
	}
	else
		OutputLogStr.append("> ---- Error: Didn't find any files to open. ----\r\n");

	// Update GUI
	DrawRoutineList();
	SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
}

//---------------------------------------------------------------------------

// Read a WAV file and parse all cues into a further parsable format for code generation
void ParseWavFile(std::string newFileName)
{
	for (int i = 0; i < NUM_A_PINS; ++i)
	{
		std::ostringstream os;
		os << "bChA[" << (i + 1) << "] = " << (bchA[i] ? "false\n" : "true\n");
		OutputDebugStringA(os.str().c_str());
	}

	OutputDebugStringA("---------");

	fileName = newFileName;
	ReadCueListFromAudioFile(fileName);
	std::string strCueInfo = GetCueInfo();
	std::string strRoutineName = ExtractFileNameCustom(fileName); strRoutineName = TrimAllWhitespace(strRoutineName.substr(0, strRoutineName.find(".wav")));

	// Check for duplicate routine names
	std::map<std::string, RoutineGUI>::iterator it;
	it = Routines.find(strRoutineName);
	if (it != Routines.end())
	{
		OutputLogStr.append("> Warning: Routine with name '").append(strRoutineName).append("' already found. Renaming to '");
		strRoutineName = GenerateRoutineName();
		OutputLogStr.append(strRoutineName).append("' instead.").append("\r\n");
	}

	// Check if filename is a valid variable name
	else if (!IsValidVarName(strRoutineName))
	{
		OutputLogStr.append("> Warning: Filename '").append(strRoutineName).append("' cannot be used as a variable name. Renaming to '");
		strRoutineName = GenerateRoutineName();
		OutputLogStr.append(strRoutineName).append("' instead.").append("\r\n");
	}

	ParseRoutineInput(strCueInfo, strRoutineName, fileName);
}

//---------------------------------------------------------------------------

// Parses cue information into a routine
void ParseRoutineInput(std::string input, std::string routineName, std::string fileName)
{
	std::istringstream ifs(input);
	std::istringstream ifsLine;
	std::string line, word, name;
	RoutineGUI routineGUI;
	Light light;
	bool bFound;

	unsigned long time1, time2, endTime = 0;

	// Set .wav file path
	routineGUI.routine.wavFilePath = fileName;

	// Format strings
	std::string comma = ", ";
	std::string endBrace = "}, ";

	routineGUI.routine.name = routineName;

	// Skip any leading junk lines
	while (std::getline(ifs, line) && line.find("File:") == std::string::npos);

	// Check for propper start of cue information
	if (line.find("File:") == std::string::npos)
	{
		OutputLogStr.append("> ---- Error: Unable to parse input. Please check format and try again. ----\r\n");
		return;
	}

	OutputDebugStringA("---------");

	// Parse each line
	std::map<std::string, Light>::iterator it;
	while (std::getline(ifs, line))
	{
		ifsLine = std::istringstream(line);

		// Propper line format: <cueNumber>) Basic: <m:ss.mmm> - <m:ss.mmm> <LightName> <PinNumber> (<m:ss.mmm>)
		//  i.e. 1) Basic: 1:23:333 - 1:43.000 Gazebo D8 (0:23.333)
		if (ifsLine >> word && ifsLine >> word && word.compare("Basic:") == 0)
		{
			// Get start time string. Convert to millis if using format mm:ss.mmm
			if (ifsLine.peek() == EOF)
				continue;
			ifsLine >> word;
			try 
			{
				time1 = word.find(":") != std::string::npos ? GetTimeMillis(word) : std::stol(word);
			}
			catch (...)
			{
				OutputDebugStringA("Skipping tag ling after failing to find start time!\n");
				continue;
			}	

			// Skip '-'
			if (ifsLine.peek() == EOF)
				continue;
			ifsLine >> word; 
			
			// Get end time string
			if (ifsLine.peek() == EOF)
				continue;
			ifsLine >> word;
			try 
			{
				time2 = word.find(":") != std::string::npos ? GetTimeMillis(word) : std::stol(word);
			}
			catch (...)
			{
				OutputDebugStringA("Skipping tag ling after failing to find end time!\n");
				continue;
			}

			// Get routine name string
			if (ifsLine.peek() == EOF)
				continue;
			ifsLine >> name;

			// Check for existing light
			it = routineGUI.routine.lights.find(name);
			bFound = it != routineGUI.routine.lights.end();

			// Create new light if none found
			light = !bFound ? Light() : it->second;
			light.name = name;

			// Check for special ALL_ON or ALL_OFF light
			if (name.compare("ALL_OFF") == 0)
				light.pin = "P_ALL_OFF";
			else if (name.compare("ALL_ON") == 0)
				light.pin = "P_ALL_ON";
			else if (ifsLine.peek() == EOF)
				continue;
			else
				ifsLine >> light.pin;

			bool* bch = GetBoolFromPinStr(light.pin);
			if (bch != nullptr)
			{
				*bch = true;
				CheckMenuItem(GetMenu(gHWND), GetUIDFromPinStr(light.pin), *bch ? MF_CHECKED : MF_UNCHECKED);
			}
			//else
			//{
			//	OutputDebugStringA("No pin found for tag, skipping...\n");
			//	continue;
			//}

			// Create OnTime array
			std::ostringstream os;
			os << "{" << time1 << comma << time2 << endBrace;

			// Create OnTimeLowPrecision array
			std::ostringstream osLP;
			osLP << "{" << (time1 / 100) << comma << (time2 / 100) << endBrace;

			// Add to list of OnTimes
			light.onTimes.append(os.str());
			light.onTimesLowPrecision.append(osLP.str());
			light.numberOfTimes++;

			// Erase existing light so that the updated one is added porpperly
			if (bFound)
				routineGUI.routine.lights.erase(it);
			routineGUI.routine.lights.insert(std::pair<std::string, Light>(name, light));

			// Update the endTime of the routine if newly found light has a longer endTime than currently recorded one
			if (time2 > endTime)
				endTime = time2;
		}
	}

	for (int i = 0; i < NUM_A_PINS; ++i)
	{
		std::ostringstream os;
		os << "bChA[" << (i + 1) << "] = " << (bchA[i] ? "false\n" : "true\n");
		OutputDebugStringA(os.str().c_str());
	}

	OutputDebugStringA("---------");

	routineGUI.routine.endTime = endTime;

	// Ensure that nothing went wrong during parsing and at least one light was parsed
	if (routineGUI.routine.lights.size() <= 0)
	{
		OutputLogStr.append("> Warning: Routine found with no lights. Ensure formatting is correct.\r\n");
		return;
	}

	// Check for duplicate routine names
	std::map<std::string, RoutineGUI>::iterator itRoutine;
	itRoutine = Routines.find(routineGUI.routine.name);
	if (itRoutine == Routines.end()) {
		std::ostringstream outputSream;
		outputSream << "> Added routine \"" << routineGUI.routine.name << "\" containing " << routineGUI.routine.lights.size() << " lights\r\n";
		OutputLogStr.append(outputSream.str());
	}
	else {
		OutputLogStr.append("> Warning: Already found routine with name \"");
		OutputLogStr.append(routineGUI.routine.name);
		OutputLogStr.append("\". Old routine will be overwritten.\r\n");
	}

	// Add closing curly brace to end of OnTimes array
	std::map<std::string, Light>::iterator itTemp;
	for (itTemp = routineGUI.routine.lights.begin(); itTemp != routineGUI.routine.lights.end(); ++itTemp)
	{
		itTemp->second.onTimes.pop_back();
		itTemp->second.onTimes.pop_back();
		itTemp->second.onTimes.append("}");

		itTemp->second.onTimesLowPrecision.pop_back();
		itTemp->second.onTimesLowPrecision.pop_back();
		itTemp->second.onTimesLowPrecision.append("}");
	}

	Routines.insert(std::pair<std::string, RoutineGUI>(routineGUI.routine.name, routineGUI));
	OrderTracker.routineNames.push_back(routineGUI.routine.name);
}

//---------------------------------------------------------------------------

// Genrates the Arduino code given the routines and options selected
std::string GenerateCode()
{
	// Format strings
	std::string comma = Options.bPrettyPrint ? ", " : ",";
	std::string endBrace = Options.bPrettyPrint ? "}, " : "},";
	std::string extraLine = Options.bPrettyPrint ? "\r\n" : "";
	
	std::ostringstream outputString, osRoutineArray;
	int i;

	// Constants and #Defines 
	outputString << "#define ulong unsigned long\r\n#define ON " << (Options.bSwapOnOffValues ? 0 : 1) << "\r\n#define OFF " << (Options.bSwapOnOffValues ? 1 : 0) <<"\r\n#define P_ALL_OFF -1\r\n#define P_ALL_ON -2\r\n";
	if (Options.bPrettyPrint)
		outputString << "// PINS\r\n";
	if (!Options.motionSensorPin.empty())
		outputString << "#define PMotionSense " << Options.motionSensorPin << "\r\n";
	outputString << "#define MP3SkipPin " << Options.mp3SkipPin << "\r\n";
	if (!Options.mp3VolumePin.empty())
		outputString << "#define MP3VolumePin " << Options.mp3VolumePin << "\r\n";
	if (!Options.trainPinLeft.empty())
		outputString << "#define TrainPin " << Options.trainPinLeft << "\r\n";
	if (Options.bDebugLights)
		outputString << "#define DEBUG\r\n";
	if (Options.bDebugTrain)
		outputString << "#define DEBUG_TRAIN\r\n";
	if (Options.bDebugSkipRoutine)
		outputString << "#define DEBUG_SKIP_ROUTINE\r\n";
	outputString << "#define D2 2\r\n" << "#define D3 3\r\n" << "#define D4 4\r\n" << "#define D5 5\r\n" << "#define D6 6\r\n" << "#define D7 7\r\n" << "#define D8 8\r\n";
	outputString << "#define D9 9\r\n" << "#define D10 10\r\n" << "#define D11 11\r\n" << "#define D12 12\r\n" << "#define D13 13\r\n";
	if (Options.bPrettyPrint)
		outputString << "\r\n";

	outputString <<	"bool bAllLightsOn = false;\r\n#define bRandomizeRoutineOrder " << (Options.bRandomizeRoutineOrder ? "true" : "false") << "\r\n";
	int numLights = 0;
	int temp = 0;
	for (i = 0; i < NUM_A_PINS; i++)
		if (bchA[i] && !Options.IsTrainPin(std::string("A").append(std::to_string(i))))
			numLights++;
	for (i = 0; i < NUM_D_PINS; i++)
		if (bchD[i] && !Options.IsTrainPin(std::string("D").append(std::to_string(i+2))))
			numLights++;
	outputString << "#define NUM_LIGHTS " << numLights << "\r\n";
	outputString << "#define NUM_ROUTINES " << Routines.size() << "\r\n";
	outputString << "int AllLights[NUM_LIGHTS] = {";
	if (bchA[0] && !Options.IsTrainPin("A0"))
		outputString << "A0" << (++temp < numLights? "," : "");
	if (bchA[1] && !Options.IsTrainPin("A1"))
		outputString << "A1" << (++temp < numLights ? "," : "");
	if (bchA[2] && !Options.IsTrainPin("A2"))
		outputString << "A2" << (++temp < numLights ? "," : "");
	if (bchA[3] && !Options.IsTrainPin("A3"))
		outputString << "A3" << (++temp < numLights ? "," : "");
	if (bchA[4] && !Options.IsTrainPin("A4"))
		outputString << "A4" << (++temp < numLights ? "," : "");
	if (bchA[5] && !Options.IsTrainPin("A5"))
		outputString << "A5" << (++temp < numLights ? "," : "");
	if (!Options.bUseLegacyA6)
		if (bchA[6] && !Options.IsTrainPin("A6"))
			outputString << "A6" << (++temp < numLights ? "," : "");
	if (!Options.bUseLegacyA7)
		if (bchA[7] && !Options.IsTrainPin("A7"))
			outputString << "A7" << (++temp < numLights ? "," : "");

	if (bchD[0] && !Options.IsTrainPin("D2"))
		outputString << "D2" << (++temp < numLights ? "," : "");
	if (bchD[1] && !Options.IsTrainPin("D3"))
		outputString << "D3" << (++temp < numLights ? "," : "");
	if (bchD[2] && !Options.IsTrainPin("D4"))
		outputString << "D4" << (++temp < numLights ? "," : "");
	if (bchD[3] && !Options.IsTrainPin("D5"))
		outputString << "D5" << (++temp < numLights ? "," : "");
	if (bchD[4] && !Options.IsTrainPin("D6"))
		outputString << "D6" << (++temp < numLights ? "," : "");
	if (bchD[5] && !Options.IsTrainPin("D7"))
		outputString << "D7" << (++temp < numLights ? "," : "");
	if (bchD[6] && !Options.IsTrainPin("D8"))
		outputString << "D8" << (++temp < numLights ? "," : "");
	if (bchD[7] && !Options.IsTrainPin("D9"))
		outputString << "D9" << (++temp < numLights ? "," : "");
	if (bchD[8] && !Options.IsTrainPin("D10"))
		outputString << "D10" << (++temp < numLights ? "," : "");
	if (bchD[9] && !Options.IsTrainPin("D11"))
		outputString << "D11" << (++temp < numLights ? "," : "");
	if (bchD[10] && !Options.IsTrainPin("D12"))
		outputString << "D12" << (++temp < numLights ? "," : "");
	if (bchD[11] && !Options.IsTrainPin("D13"))
		outputString << "D13";
	outputString << "};\r\n";

	// Even when not randomizing, we need these because my code is dumb
	outputString << "bool all_used_up = false;\r\nbool used_routine[NUM_ROUTINES];\r\n";
	
	// Motor current variables
	if (!Options.trainPinLeft.empty()) {
		outputString << "uint8_t motor_current_limit = 35;\r\nuint8_t motor_current_limit_L2R = 24;\r\nuint8_t motor_current_limit_R2L = 24;\r\n";
	}

	// NEW CODE GENERATED
	outputString << "ulong StartTime = 0, DeltaTime = 0;\r\nint CurrentRoutine = 0;\r\n" << extraLine;

	// Structures
	if (Options.bPrettyPrint)
	{
		outputString << "/* ----------------------------------------------------------------------------------------------------\r\n";
		outputString << " *                                           Data Structures                                           \r\n";
		outputString << " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}
	outputString << "typedef struct OnTime\r\n{\r\n	";
	if (Options.bUseLowPrecisionTimes)
		outputString << "unsigned int Start;\r\n	unsigned int End;";
	else
		outputString << "unsigned long Start;\r\n	unsigned long End;";
	outputString << "\r\n} OnTime;\r\n\r\ntypedef struct Light\r\n{ \r\n	int Pin; \r\n	 OnTime* Times; \r\n	int NumberOfOnTimes; \r\n	int State; \r\n } Light; \r\n\r\ntypedef struct Routine\r\n{\r\n	Light** Lights;\r\n	int NumberOfLights;\r\n	" << (Options.bUseLowPrecisionTimes ? "unsigned int" : "unsigned long") << " RoutineTime;\r\n} Routine;\r\n";
	outputString << extraLine;

	// Routine Variables
	if (Options.bPrettyPrint)
	{
		outputString << "/* ----------------------------------------------------------------------------------------------------\r\n";
		outputString << " *                                       Light/Routine Variables                                       \r\n";
		outputString << " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}

	std::map<std::string, Light>::iterator itLights;
	std::map<std::string, RoutineGUI>::iterator itRoutines;
	for (itRoutines = Routines.begin(); itRoutines != Routines.end(); ++itRoutines)
	{
		std::ostringstream osLightArray;
		outputString << "//" << itRoutines->second.routine.name << "\r\n";
		for (itLights = itRoutines->second.routine.lights.begin(); itLights != itRoutines->second.routine.lights.end(); ++itLights)
		{
			outputString << "OnTime " << itRoutines->second.routine.name << "_" << itLights->second.name << "_OnTimes[" << itLights->second.numberOfTimes << "] = {" << (Options.bUseLowPrecisionTimes ? itLights->second.onTimesLowPrecision : itLights->second.onTimes) << ";\r\n";
			outputString << "Light " << itRoutines->second.routine.name << "_" << itLights->second.name << " = {" << itLights->second.pin << comma << itRoutines->second.routine.name;
			outputString << "_" << itLights->second.name << "_OnTimes" << comma << itLights->second.numberOfTimes << comma << "OFF};\r\n";
			if (Options.bPrettyPrint)
				outputString << "\r\n";

			osLightArray << "&" << itRoutines->second.routine.name << "_" << itLights->second.name << comma;
		}
		outputString << "Light* " << itRoutines->second.routine.name << "_Lights[" << itRoutines->second.routine.lights.size() << "] = {";
		outputString << osLightArray.str().substr(0, osLightArray.str().size() - (Options.bPrettyPrint ? 2 : 1)) << "};\r\n"; //CHECK  THIS
		outputString << "Routine " << itRoutines->second.routine.name << " = {" << itRoutines->second.routine.name << "_Lights" << comma << itRoutines->second.routine.lights.size() << comma;
		outputString << (Options.bUseLowPrecisionTimes ? itRoutines->second.routine.endTime/100 : itRoutines->second.routine.endTime) << "};\r\n";
		if (Options.bPrettyPrint)
			outputString << "\r\n";

		osRoutineArray << "&" << itRoutines->second.routine.name << comma;	
	}

	std::string routinesString = OrderTracker.getRoutinesString(Options.bUseHalloweenMP3Controls);
	outputString << "Routine* " << "routines[NUM_ROUTINES] = {" << routinesString << "};\r\n" << extraLine; //CHECK  THIS

	if (Options.bPrettyPrint)
	{
		outputString << "/* ----------------------------------------------------------------------------------------------------\r\n *                                         Town Scene Functions                                        \r\n * ---------------------------------------------------------------------------------------------------- */\r\n";
	}

	// Check Light
	outputString << "/** \r\n"
		<< " * Turns a light on if DeltaTime is within one of the light's \"on times\". \r\n"
		<< " * Otherwise, the light is turned off.\r\n"
		<< " *   - light: The light to check\r\n"
		<< " *   Returns: Whether the light was turned on\r\n"
		<< " **/\r\n"
		<< "bool CheckLight(Light* light)\r\n"
		<< "{\r\n"
		<< "    int i;\r\n"
		<< "    for (i = 0; i < light->NumberOfOnTimes; i++)\r\n"
		<< "    {\r\n"
		<< "        if (light->Pin == P_ALL_OFF || light->Pin == P_ALL_ON)\r\n"
		<< "            return false;\r\n"
		<< "        if (DeltaTime >= "
		<< (Options.bUseLowPrecisionTimes
			? "((ulong)light->Times[i].Start * 100)"
			: "light->Times[i].Start")
		<< " && DeltaTime < "
		<< (Options.bUseLowPrecisionTimes
			? "((ulong)light->Times[i].End * 100)"
			: "light->Times[i].End")
		<< ")\r\n"
		<< "        {\r\n"
		<< "            #ifdef DEBUG\r\n"
		<< "            if (light->State == OFF) \r\n"
		<< "            {\r\n"
		<< "                Serial.print(\"Turning on light : \");\r\n"
		<< "                Serial.println(light->Pin);\r\n"
		<< "            }\r\n"
		<< "            light->State = ON;\r\n"
		<< "            #endif\r\n"
		<< "            digitalWrite(light->Pin, ON);\r\n"
		<< "            return true;\r\n"
		<< "        }\r\n"
		<< "    }\r\n"
		<< "    digitalWrite(light->Pin, OFF);\r\n"
		<< "\r\n"
		<< "    #ifdef DEBUG\r\n"
		<< "    if (light->State == ON)\r\n"
		<< "    {\r\n"
		<< "        Serial.print(\"Turning off light : \");\r\n"
		<< "        Serial.println(light->Pin);\r\n"
		<< "    }\r\n"
		<< "    light->State = OFF;\r\n"
		<< "    #endif\r\n"
		<< "    return false;\r\n"
		<< "}\r\n";
	//outputString << "/** Turns a light on if DeltaTime is within one of the light's \"on times\". Otherwise, the light\r\n *  is turned off.\r\n *   - light: The light to check\r\n *   Returns: Whether the light was turned on\r\n **/\r\nbool CheckLight(Light* light)\r\n{\r\n	int i;\r\n	for (i = 0; i < light->NumberOfOnTimes; i++)\r\n	{\r\n		if (light->Pin == P_ALL_OFF || light->Pin == P_ALL_ON)\r\n		return false;\r\n		if (DeltaTime >= "
	//	<< (Options.bUseLowPrecisionTimes 
	//		? "((ulong)light->Times[i].Start * 100)" 
	//		:  "light->Times[i].Start") << " && DeltaTime < " 
	//	<< (Options.bUseLowPrecisionTimes 
	//		? "((ulong)light->Times[i].End * 100)" 
	//		: "light->Times[i].End") << ")\r\n		{\r\n			#ifdef DEBUG\r\n			if (light->State == OFF) \r\n			{\r\n				Serial.print(\"Turning on light : \");\r\n				Serial.println(light->Pin);\r\n			}\r\n			light->State = ON;\r\n			#endif\r\n			digitalWrite(light->Pin, ON);\r\n			return true;\r\n		}\r\n	}\r\n	digitalWrite(light->Pin, OFF);\r\n\r\n	#ifdef DEBUG\r\n	if (light->State == ON)\r\n	{\r\n		Serial.print(\"Turning off light : \");\r\n		Serial.println(light->Pin);\r\n	}\r\n	light->State = OFF;\r\n	#endif\r\n	return false;\r\n}\r\n";
	outputString << extraLine;

	if (!Options.bUseHalloweenMP3Controls) {
		// Skip To Routine
		outputString << "/** Applies the proper number of Skip commands to the MP3 player in order to go from the current\r\n * 	track to the desired track.\r\n *   Returns: array position of next routine\r\n **/\r\nint SkipToRoutine()\r\n{";
		
		// v2.0 - new non-repeating randomization implementation
		outputString << "    if (all_used_up) {\r\n"
			<< "        #ifdef DEBUG_SKIP_ROUTINE\r\n"
			<< "        Serial.print(\"all_used_up :\");\r\n"
			<< "        Serial.println(all_used_up);\r\n"
			<< "        #endif\r\n"
			<< "        for (int ii = 0; ii < NUM_ROUTINES; ii++) {\r\n"
			<< "            used_routine[ii] = false;\r\n"
			<< "        }\r\n"
			<< "        all_used_up = false;\r\n"
			<< "    }\r\n"
			<< "    int nextRoutine = bRandomizeRoutineOrder\r\n"
			<< "        ? CurrentRoutine\r\n"
			<< "        : CurrentRoutine + 1 == NUM_ROUTINES ? 0 : CurrentRoutine + 1;\r\n"
			<< "    int numberOfSkips = 1;\r\n"
			<< "\r\n"
			<< "    if (bRandomizeRoutineOrder && NUM_ROUTINES > 1) {\r\n"
			<< "        while (nextRoutine == CurrentRoutine || used_routine[nextRoutine])\r\n"
			<< "            nextRoutine = random(0, NUM_ROUTINES);\r\n"
			<< "        used_routine[nextRoutine] = true;\r\n"
			<< "        numberOfSkips = nextRoutine < CurrentRoutine\r\n"
			<< "            ? NUM_ROUTINES - CurrentRoutine + nextRoutine\r\n"
			<< "            : nextRoutine - CurrentRoutine;\r\n"
			<< "\r\n"
			<< "        all_used_up = true;\r\n"
			<< "        for (int i = 0; i < NUM_ROUTINES; i++) {\r\n"
			<< "            if (!used_routine[i]) {\r\n"
			<< "                all_used_up = false;\r\n"
			<< "                break;\r\n"
			<< "            }\r\n"
			<< "        }\r\n"
			<< "\r\n"
			<< "        #ifdef DEBUG_SKIP_ROUTINE\r\n"
			<< "        Serial.print(\"Current routine :\");\r\n"
			<< "        Serial.println(CurrentRoutine);\r\n"
			<< "        Serial.print(\"Routine \");\r\n"
			<< "        Serial.print(nextRoutine);\r\n"
			<< "        Serial.print(\" selected(skipping \");\r\n"
			<< "        Serial.print(numberOfSkips);\r\n"
			<< "        Serial.println(\" times)\");\r\n"
			<< "        #endif\r\n"
			<< "    }\r\n"
			<< "    for (int i = 0; i < numberOfSkips; i++) {\r\n"
			<< "		#ifdef DEBUG_SKIP_ROUTINE\r\n"
			<< "		Serial.print(\" skipping...\");\r\n"
			<< "		#endif\r\n"
			<< "        digitalWrite(MP3SkipPin, HIGH);\r\n"
			<< "        delay(150);\r\n"
			<< "        digitalWrite(MP3SkipPin, LOW);\r\n"
			<< "        delay(250);\r\n"
			<< "    }\r\n"
			<< "	#ifdef DEBUG_SKIP_ROUTINE\r\n"
			<< "	Serial.println(\"Done skipping!\");\r\n"
			<< "	#endif\r\n"
			<< "    return nextRoutine;\r\n"
			<< "}";

		outputString << extraLine;
	}

	// Turn All Lights
	outputString << "/** Turns all lights on or off.\r\n *	- state: ON or OFF\r\n **/\r\nvoid TurnAllLights(int state)\r\n{\r\n	for (int i=0; i < NUM_LIGHTS; i++)	\r\n	{\r\n		#ifdef DEBUG\r\n		Serial.print(\"Turning light \");\r\n		Serial.print(AllLights[i]);\r\n		Serial.println(state == ON ? \" ON\" : \" OFF\");\r\n		#endif\r\n		digitalWrite(AllLights[i], state);	\r\n	}\r\n}\r\n";
	outputString << extraLine;

	// All Lights On 
	outputString << "/** Checks to see if all lights should be turned on or not.\r\n *	- allOnLight: Light pointer containing all OnTimes for All Lights On\r\n **/\r\nbool AllLightsOn(Light* allOnLight)\r\n{\r\n	for (int i = 0; i < allOnLight->NumberOfOnTimes; i++)\r\n	{\r\n		if (DeltaTime >= " << (Options.bUseLowPrecisionTimes ? "((ulong)allOnLight->Times[i].Start * 100)" : "allOnLight->Times[i].Start" ) << " && DeltaTime < " << (Options.bUseLowPrecisionTimes ? "((ulong)allOnLight->Times[i].End * 100)" : "allOnLight->Times[i].End" ) << ")\r\n		{\r\n			#ifdef DEBUG\r\n			if (allOnLight->State == OFF) \r\n			{\r\n				Serial.println(\"Turning all lights ON.\");\r\n			}\r\n			allOnLight->State = ON;\r\n			#endif\r\n			TurnAllLights(ON);\r\n			return true;\r\n		}\r\n	}\r\n	if (bAllLightsOn)\r\n	{\r\n		#ifdef DEBUG\r\n		Serial.println(\"Turning all lights OFF.\");\r\n		allOnLight->State = OFF;	\r\n		#endif\r\n		TurnAllLights(OFF);\r\n	}\r\n	return false;\r\n}\r\n";
	outputString << extraLine;	

	// All Lights Off
	outputString << "/** Checks to see if all lights should be turned off or not. This function only sends ALL OFF command once per instance.\r\n *	- allOffLight: Light pointer containing all OnTimes for All Lights Off\r\n **/\r\nbool AllLightsOff(Light* allOffLight)\r\n{\r\n	for (int i = 0; i < allOffLight->NumberOfOnTimes; i++)\r\n	{\r\n		if (DeltaTime >= " << (Options.bUseLowPrecisionTimes ? "((ulong)allOffLight->Times[i].Start * 100)" : "allOffLight->Times[i].Start") << " && DeltaTime < " << (Options.bUseLowPrecisionTimes ? "((ulong)allOffLight->Times[i].End * 100)" : "allOffLight->Times[i].End") << ")\r\n		{\r\n			#ifdef DEBUG\r\n			Serial.println(\"Turning all lights OFF.\");\r\n			#endif\r\n			TurnAllLights(OFF);\r\n			allOffLight->Times[i].End = 0;\r\n			return true;\r\n		}\r\n	}\r\n	return false;\r\n}\r\n";
	outputString << extraLine;

	// Find Light
	outputString << "/** Finds a light object in a given routine based on pin number.\r\n *	- routine: the routine to search through\r\n *	- pin: the pin of the light to find\r\n *	Returns: the light or NULL if no light found\r\n **/\r\nLight* FindLight(Routine* routine, int pin)\r\n{\r\n	for (int i=0; i < routine->NumberOfLights; i++)\r\n	{\r\n		if (routine->Lights[i]->Pin == pin)\r\n		{\r\n			#ifdef DEBUG\r\n			Serial.print(\"Found light with pin : \");\r\n			Serial.println(pin);\r\n			#endif\r\n			return routine->Lights[i];\r\n		}\r\n	}\r\n	#ifdef DEBUG\r\n	Serial.print(\"No light with pin \");\r\n	Serial.print(pin);\r\n	Serial.println(\" found...\");\r\n	#endif\r\n	return NULL;\r\n}\r\n";
	outputString << extraLine;

	if (Options.bPrettyPrint)
	{
		outputString << "/* ----------------------------------------------------------------------------------------------------\r\n";
		outputString << " *                                          Arduino Functions                                          \r\n";
		outputString << " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}
	bool* pMotionSenseBoolPin = GetBoolFromPinStr(Options.motionSensorPin);
	// Setup
	outputString << "void setup()\r\n{\r\n	Serial.begin(9600);\r\n";
	for (int i = 0; i < NUM_A_PINS; ++i)
	{
		// Skip A6 if legacy option enabled
		if (i == 5 && Options.bUseLegacyA6)
			continue;

		if (bchA[i])
			outputString << "	pinMode(A" << i << ", " << (pMotionSenseBoolPin == &(bchA[i]) ? "INPUT" : "OUTPUT") << ");\r\n";
	}

	for (int i = 0; i < NUM_D_PINS; ++i)
		if (bchD[i])
			outputString << "	pinMode(D" << (i + 2) << ", " << (pMotionSenseBoolPin == &(bchD[i]) ? "INPUT" : "OUTPUT") << ");\r\n";

	// Setup motion sensor pin
	if (!Options.motionSensorPin.empty())
		outputString << "	pinMode(" << Options.motionSensorPin << ", INPUT_PULLUP);\r\n";

	// Setup skip button pin
	if (!Options.mp3SkipPin.empty())
		outputString << "	pinMode(" << Options.mp3SkipPin << ", OUTPUT);\r\n";
	
	// Setup mp3 volume pin
	if (!Options.mp3VolumePin.empty())
		outputString << "	pinMode(" << Options.mp3VolumePin << ", OUTPUT);\r\n";

	//outputString << "	pinMode(D2, OUTPUT);\r\n	pinMode(D3, OUTPUT);\r\n	pinMode(D4, OUTPUT);\r\n	pinMode(D5, OUTPUT);\r\n	pinMode(D6, OUTPUT);\r\n	pinMode(D7, OUTPUT);\r\n	pinMode(D8, OUTPUT);\r\n	pinMode(D9, OUTPUT);\r\n	pinMode(D10, OUTPUT);\r\n	pinMode(D11, OUTPUT);\r\n	pinMode(D12, OUTPUT);\r\n	pinMode(D13, OUTPUT);\r\n	pinMode(A0, OUTPUT);\r\n	pinMode(A1, OUTPUT);\r\n	pinMode(A2, OUTPUT);\r\n	pinMode(A3, OUTPUT);\r\n	pinMode(A4, OUTPUT);\r\n	pinMode(A5, OUTPUT);\r\n	pinMode(A6, INPUT);\r\n	pinMode(A7, INPUT_PULLUP);\r\n	TurnAllLights(ON);\r\n";
	
	if (!Options.motorVoltagePin.empty())
		outputString << "	pinMode(" << Options.motorVoltagePin << ", INPUT);\r\n";

	if (!Options.randomSeedPin.empty())
		outputString << "	pinMode(" << Options.randomSeedPin << ", INPUT);\r\n";

	if (Options.bUseLegacyA6)
		outputString << "	pinMode(A6, INPUT);\r\n";
	if (Options.bUseLegacyA7)
		outputString << "	pinMode(A7, INPUT_PULLUP);\r\n";

	// In the event the option to "randomly select track" is checked, we'll need to initialize randomSeed
	if (Options.bRandomizeRoutineOrder && !Options.randomSeedPin.empty())
		outputString << "	randomSeed(analogRead(" << Options.randomSeedPin << "));\r\n";

	outputString << "	TurnAllLights(ON);\r\n";
	if (!Options.trainPinLeft.empty()) {
		const int maxTrainInit = Options.trainResetDuration <= 0 ? 20000 : Options.trainResetDuration;
		//if (Options.trainResetDuration <= 0) {
			// Auto-initialize train using motor avg
		outputString << "    #ifdef DEBUG_TRAIN\r\n"
			<< "        Serial.println(\"Resetting train to designated side...\");\r\n"
			<< "    #endif\r\n"
			<< "    digitalWrite(" << Options.trainPinRight << ", OFF);      // Make sure train direction is L to R\r\n"
			<< "    delay(250);\r\n"
			<< "    digitalWrite(TrainPin, ON); // S/B A5\r\n"
			<< "    delay(10);\r\n"
			<< "    int startTime = millis();  // Record the start time\r\n"
			<< "    int maxDuration = " << maxTrainInit << ";\r\n"
			<< "    unsigned int time1;\r\n"
			<< "    unsigned int time2;\r\n"
			<< "    uint8_t motoravgmax = 0;\r\n"
			<< "    uint8_t motoravgmin = 255;\r\n"
			<< "    uint8_t motorAvg = 0;\r\n"
			<< "    uint8_t motorAvg1 = 0;\r\n";
		if (Options.bRandomizeRoutineOrder) {
			outputString << "    for (int ii = 0; ii < NUM_ROUTINES; ii++) {  // resets used routine array to all false\r\n"
				<< "        used_routine[ii] = false;\r\n"
				<< "    }\r\n"
				<< "    all_used_up = false;        // reset all routine used variable\r\n";
		}
		outputString << "    while ((millis() - startTime < maxDuration) && (motorAvg <= motor_current_limit)) {\r\n"
			<< "        motorAvg = 0;\r\n"
			<< "        motorAvg1 = 0;\r\n"
			<< "        time1 = millis();\r\n"
			<< "        for (int i = 0; i < 500; i++) {\r\n"
			<< "            motorAvg1 = analogRead("<< Options.motorVoltagePin <<");\r\n"
			<< "            motorAvg = (motorAvg + motorAvg1) / 2;\r\n"
			<< "            delay(1); // Small delay between readings to ensure stable sampling\r\n"
			<< "        }\r\n"
			<< "        time2 = millis() - time1;\r\n"
			<< "\r\n"
			<< "        #ifdef DEBUG_TRAIN\r\n"
			<< "            Serial.print(\"Time for averaging: \");\r\n"
			<< "            Serial.println(time2);\r\n"
			<< "            if (motorAvg >= motoravgmax)\r\n"
			<< "                motoravgmax = motorAvg;\r\n"
			<< "            if (motorAvg <= motoravgmin)\r\n"
			<< "                motoravgmin = motorAvg;\r\n"
			<< "            Serial.print(\"Max Average Current: \");\r\n"
			<< "            Serial.print(motoravgmax);\r\n"
			<< "            Serial.print(\", Min Average Current: \");\r\n"
			<< "            Serial.print(motoravgmin);\r\n"
			<< "            Serial.print(\", Average Motor Current: \");\r\n"
			<< "            Serial.println(motorAvg);\r\n"
			<< "        #endif\r\n"
			<< "    }\r\n"
			<< "\r\n"
			<< "    digitalWrite(TrainPin, OFF);\r\n"
			<< "    digitalWrite(" << Options.trainPinRight << ", ON);       // Back train off the stop a bit\r\n"
			<< "    delay(250);\r\n"
			<< "    digitalWrite(" << Options.trainPinRight << ", OFF);\r\n"
			<< "    while (millis() - startTime < maxDuration) {  // Use this remaining train initialization time for all lights on at startup\r\n"
			<< "        delay(100);\r\n"
			<< "    }\r\n"
			<< "    #ifdef DEBUG_TRAIN\r\n"
			<< "        Serial.println(\"Finished resetting train!\");\r\n"
			<< "    #endif";
			
			//outputString << "	#ifdef DEBUG_TRAIN\r\n	Serial.println(\"Resetting train to designated side...\");\r\n	#endif\r\n"
			//	<< "	digitalWrite(" << Options.trainPinRight << ", OFF);\r\n	delay(250);\r\n	digitalWrite(TrainPin, ON);\r\n	delay(10);\r\n"
			//	<< "	int startTime = millis();\r\n	int maxDuration = 20000;\r\n	unsigned int time1, time2;\r\n"
			//	<< "	uint8_t motoravgmax = 0, motoravgmin = 255, motorAvg = 0, motorAvg1 = 0;\r\n"
			//	<< "	for (int ri = 0; ri < NUM_ROUTINES; ri++) {\r\n"
			//	<< "		used_routine[ri] = false;\r\n"
			//	<< "	}\r\n	all_used_up = false;\r\n"
			//	<< "	while ((millis() - startTime < maxDuration) && (motorAvg <= motor_current_limit)) {\r\n"
			//	<< "		motorAvg = 0;\r\n		motorAvg1 = 0;\r\n		time1 = millis();\r\n		for (int i = 0; i < 500; i++) {\r\n"
			//	<< "			motorAvg1 = analogRead(" << Options.trainMotorPin << ");\r\n			motorAvg = (motorAvg + motorAvg1) / 2;\r\n"
			//	<< "			delay(1);\r\n		}\r\n		time2 = millis() - time1;\r\n\r\n		#ifdef DEBUG_TRAIN\r\n"
			//	<< "		Serial.print(\"Time for averaging: \");\r\n		Serial.println(time2);\r\n"
			//	<< "		motorAvg = motorAvg / 500;\r\n		if (motorAvg >= motoravgmax)\r\n			motoravgmax = motorAvg;\r\n"
			//	<< "		if (motorAvg <= motoravgmin)\r\n			motoravgmin = motorAvg;\r\n"
			//	<< "		Serial.print(\" , Max Average Current: \");\r\n		Serial.print(motoravgmax);\r\n		Serial.print(\" , Min Average Current: \")\r\n"
			//	<< "		Serial.print(motoravgmin);\r\n		Serial.print(\"Average Motor Current: \");\r\n		Serial.println(motorAvg);\r\n"
			//	<< "		#endif\r\n		}\r\n\r\n		digitalWrite(TrainPin, OFF);\r\n		digitalWrite(" << Options.trainPinRight << ", ON);\r\n"
			//	<< "		delay(250);\r\n		digitalWrite(" << Options.trainPinRight << ", OFF);\r\n		while (millis() - startTime < maxDuration) {\r\n"
			//	<< "			delay(100);\r\n		}\r\n		#ifdef DEBUG_TRAIN\r\n		Serial.println(\"Finished resetting train!\");\r\n		#endif\r\n";
		//}
		//else {
		//	// Initialize train using hardcoded reset duration
		//	outputString << "	#ifdef DEBUG_TRAIN\r\n	Serial.println(\"Resetting train to designated side...\");\r\n	#endif\r\n	digitalWrite(TrainPin, ON);\r\n	delay(" << Options.trainResetDuration << ");\r\n	digitalWrite(TrainPin, OFF);\r\n	#ifdef DEBUG\r\n	Serial.println(\"Finished resetting train!\");\r\n	#endif\r\n";
		//}
	}
	else {
		// Ignore train setup - induce 7s delay
		outputString << "	delay(7000);\r\n";
	}
	outputString << "	TurnAllLights(OFF);\r\n";
	outputString << "}\r\n";
	outputString << extraLine;

	

	// Loop
	outputString << "void loop()\r\n{\r\n";
	if (!Options.motionSensorPin.empty())
		outputString << "\twhile (analogRead(PMotionSense) < 500) ;\r\n";
	if (Options.bUseHalloweenMP3Controls) {
		outputString << "	CurrentRoutine = 0;\r\n	digitalWrite(MP3SkipPin, HIGH);\r\n";
	}
	else {
		outputString << "	CurrentRoutine = SkipToRoutine();\r\n";
	}

	// Commented out debug code
	outputString << "/* ---------------------------------------------------------------------------------------------------- */\r\n"
		<< "// This code is used only to force a specific routine to run.\r\n"
		<< "// 1 skip = Set 1, Routine 1\r\n"
		<< "    // CurrentRoutine = 1;\r\n"
		<< "    // int numberOfSkips = 1;\r\n"
		<< "    // for (int i = 0; i < numberOfSkips; i++)\r\n"
		<< "    // {\r\n"
		<< "    //     Serial.print(\" skipping...)\");\r\n"
		<< "    //     digitalWrite(MP3SkipPin, HIGH);\r\n"
		<< "    //     delay(150);\r\n"
		<< "    //     digitalWrite(MP3SkipPin, LOW);\r\n"
		<< "    //     delay(250);\r\n"
		<< "    // }\r\n"
		<< "    // Serial.println(\"Done skipping)\");\r\n"
		<< "/* ---------------------------------------------------------------------------------------------------- */\r\n";

	outputString << "	#ifdef DEBUG_SKIP_ROUTINE\r\n"
		<< "	Serial.print(\"Current Routine: \");\r\n"
		<< "	Serial.println(CurrentRoutine);\r\n"
		<< "	#endif\r\n";

	outputString << "	StartTime = millis();\r\n	Light* allOffLight = FindLight(routines[CurrentRoutine], P_ALL_OFF);\r\n	Light* allOnLight = FindLight(routines[CurrentRoutine], P_ALL_ON);\r\n	do\r\n	{\r\n		DeltaTime = millis() - StartTime;\r\n";

	if (!Options.mp3VolumePin.empty())
		outputString << "		digitalWrite(MP3VolumePin, DeltaTime < 7000 ? HIGH : LOW);\r\n";
	outputString << "		if (allOffLight != NULL)\r\n			AllLightsOff(allOffLight);\r\n		if (allOnLight != NULL)\r\n			bAllLightsOn = AllLightsOn(allOnLight);\r\n		for (int i=0; i < routines[CurrentRoutine]->NumberOfLights; i++)\r\n			CheckLight(routines[CurrentRoutine]->Lights[i]);\r\n\r\n";
	
	// Train train motor state
	if (!Options.trainPinLeft.empty()) {
		outputString << "		int pinStateL2R = digitalRead(" << Options.trainPinLeft << ");\r\n"
			<< "		int pinStateR2L = digitalRead(" << Options.trainPinRight << ");\r\n"
			<< "		if (pinStateR2L == HIGH || pinStateL2R == HIGH) {\r\n"
			<< "			if (pinStateL2R == HIGH)\r\n				motor_current_limit = motor_current_limit_L2R;\r\n"
			<< "			if (pinStateR2L == HIGH)\r\n				motor_current_limit = motor_current_limit_R2L;\r\n"
			<< "			delay(100);\r\n			uint8_t motorAvg = 0;\r\n			uint8_t motorAvg1 = 0;\r\n"
			<< "			for (int mi = 0; mi < 50; mi++) {\r\n"
			<< "				motorAvg1 = analogRead(" << Options.motorVoltagePin << ");\r\n"
			<< "				delay(1);\r\n				if (motorAvg1 >= 25) motorAvg1 = 25;\r\n"
			<< "				motorAvg = (motorAvg + motorAvg1) / 2;\r\n"
			<< "			}\r\n"
			<< "			#ifdef DEBUG_TRAIN\r\n			Serial.print(\"Average Motor Current: \");\r\n			Serial.println(motorAvg);\r\n			#endif\r\n\r\n"
			<< "			if (motorAvg >= motor_current_limit) {\r\n"
			<< "				pinStateL2R = digitalRead(" << Options.trainPinLeft << ");\r\n"
			<< "				pinStateR2L = digitalRead(" << Options.trainPinRight << ");\r\n"
			<< "				if (pinStateL2R == HIGH) {\r\n"
			<< "					digitalWrite(" << Options.trainPinRight << ", OFF);\r\n					digitalWrite(" << Options.trainPinLeft << ", ON);\r\n"
			<< "					delay(250);\r\n"
			<< "					digitalWrite(" << Options.trainPinLeft << ", OFF);\r\n"
			<< "					#ifdef DEBUG_TRAIN\r\n"
			<< "					Serial.println(\"Motor stopped by current monitor. Code delay of 4 seconds has begun \");\r\n"
			<< "					#endif\r\n"
			<< "					delay(2000);\r\n"
			<< "				}\r\n"
			<< "				if (pinStateR2L == HIGH) {\r\n"
			<< "					digitalWrite(" << Options.trainPinLeft << ", OFF);\r\n					digitalWrite(" << Options.trainPinRight << ", ON);\r\n"
			<< "					delay(250);\r\n"
			<< "					digitalWrite(" << Options.trainPinRight << ", OFF);\r\n"
			<< "					#ifdef DEBUG_TRAIN\r\n"
			<< "					Serial.println(\"Motor stopped by current monitor. Code delay of 4 seconds has begun \");\r\n"
			<< "					#endif\r\n"
			<< "					delay(2000);\r\n"
			<< "				}\r\n"
			<< "			}\r\n"
			<< "		}";
	}

	
	outputString << "} while (DeltaTime <= " << (Options.bUseLowPrecisionTimes ? "((ulong)routines[CurrentRoutine]->RoutineTime * 100)" : "routines[CurrentRoutine]->RoutineTime") << ");\r\n	for (int x=0; allOffLight != NULL && x < allOffLight->NumberOfOnTimes; x++)\r\n		allOffLight->Times[x].End = " << "routines[CurrentRoutine]->RoutineTime" << ";\r\n";
	if (Options.bUseHalloweenMP3Controls) {
		outputString << "	digitalWrite(MP3SkipPin, LOW);\r\n";
	}

	outputString << "}\r\n";
	outputString << extraLine;

	return outputString.str();
}

//---------------------------------------------------------------------------

unsigned int GetUIDFromPinStr(std::string pinStr)
{
	int i;
	for (i = 0; i < NUM_A_PINS; ++i)
	{
		std::ostringstream os;
		os << "A" << i;
		if (pinStr.compare(os.str()) == 0)
			return APinIds[i];
	}

	for (i = 0; i < NUM_D_PINS; ++i)
	{
		std::ostringstream os;
		os << "D" << (i + 2);
		if (pinStr.compare(os.str()) == 0)
			return DPinIds[i];
	}
}

bool* GetBoolFromPinStr(std::string pinStr)
{
	int i;
	for (i = 0; i < NUM_A_PINS; ++i)
	{
		std::ostringstream os;
		os << "A" << i;
		if (pinStr.compare(os.str()) == 0)
			return &bchA[i];
	}

	for (i = 0; i < NUM_D_PINS; ++i)
	{
		std::ostringstream os;
		os << "D" << (i + 2);
		if (pinStr.compare(os.str()) == 0)
			return &bchD[i];
	}

	return nullptr;
}

bool* GetBoolCB(unsigned int id)
{
	int i;
	for (i = 0; i < NUM_A_PINS; i++)
	{
		if (id == APinIds[i])
			return &bchA[i];
	}

	for (i = 0; i < NUM_D_PINS; i++)
	{
		if (id == DPinIds[i])
			return &bchD[i];
	}
	
	return NULL;
}

//---------------------------------------------------------------------------

bool AllSelected(int set)
{
	// Check DPins
	if (set == 1)
	{
		for (int x = 0; x < NUM_D_PINS; x++)
		{
			if (!bchD[x])
				return false;
		}
	}
	// Check APins
	else
	{
		for (int i = 0; i < NUM_A_PINS; i++)
		{
			if (!bchA[i])
				return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------

void SelectAll(int set, bool setTo, HWND hWnd)
{
	// Select all DPins
	int i;
	if (set == 1)
	{
		for (i = 0; i < NUM_D_PINS; ++i)
		{
			bchD[i] = setTo;
			CheckMenuItem(GetMenu(hWnd), DPinIds[i], setTo ? MF_CHECKED : MF_UNCHECKED);
			
		}
	}
	// Select all APins
	else
	{
		for (i = 0; i < NUM_A_PINS; i++)
		{
			bchA[i] = setTo;
			CheckMenuItem(GetMenu(hWnd), APinIds[i], setTo ? MF_CHECKED : MF_UNCHECKED);
		}
	}
}

//---------------------------------------------------------------------------

void CheckSelectedPin(int wmId, HWND hWnd)
{
	bool* bCb = GetBoolCB(wmId);
	*bCb = !*bCb;
	CheckDlgButton(hWnd, wmId, *bCb ? BST_CHECKED : BST_UNCHECKED);
}

