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
bool bFirstDraw = true;
const int NUM_ROUTINE_BUTTONS = 4;

HFONT g_HeaderFont = NULL;
HFONT g_ButtonFont = NULL;
HFONT g_OptionsHeaderFont = NULL;
HFONT g_LabelFont = NULL;
HFONT g_InputFont = NULL;

HWND gHWND;
HWND hMP3DriveLetter;
HWND hMP3Pin;
HWND hMotionSensorPin;
HWND hWifiSSID;
HWND hWifiPass;
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

// UI Constants - Spacing, Sizes, Colors, Fonts
namespace UIConstants {
	// Spacing
	const int MARGIN_SMALL = 5;
	const int MARGIN_MEDIUM = 10;
	const int MARGIN_LARGE = 15;
	const int MARGIN_XLARGE = 20;
	const int SECTION_SPACING = 30;
	const int CONTROL_SPACING = 8;
	const int LABEL_INPUT_GAP = 5;
	const int COLUMN_SPACING = 10;
	
	// Sizes
	const int BUTTON_HEIGHT_STANDARD = 30;
	const int BUTTON_HEIGHT_LARGE = 35;
	const int BUTTON_WIDTH_SMALL = 100;
	const int BUTTON_WIDTH_MEDIUM = 120;
	const int BUTTON_WIDTH_LARGE = 140;
	const int BUTTON_WIDTH_XLARGE = 200;
	const int INPUT_HEIGHT = 22;
	const int INPUT_WIDTH_PIN = 40;
	const int INPUT_WIDTH_STANDARD = 200;
	const int LABEL_HEIGHT = 20;
	const int HEADER_HEIGHT = 16;
	const int GROUP_BOX_PADDING = 20;
	
	// Fonts
	const int FONT_SIZE_HEADER = 18;
	const int FONT_SIZE_SUBHEADER = 16;
	const int FONT_SIZE_BUTTON = 16;
	const int FONT_SIZE_LABEL = 14;
	const int FONT_SIZE_INPUT = 14;
	const wchar_t* FONT_FAMILY = L"Segoe UI";
	
	// Colors (RGB values)
	const COLORREF COLOR_MY_BACKGROUND = RGB(240, 240, 240);
	const COLORREF COLOR_SECTION_BG = RGB(255, 255, 255);
	const COLORREF COLOR_BORDER = RGB(200, 200, 200);
	const COLORREF COLOR_TEXT = RGB(0, 0, 0);
	const COLORREF COLOR_TEXT_SECONDARY = RGB(64, 64, 64);
}


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
INT_PTR CALLBACK	EditRoutineCB(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
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
	   RoutineWidth = width / 2 - (RoutineBtnWidth*NUM_ROUTINE_BUTTONS) - 10;
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

std::string helper_FormatName(const std::string& input) {
	std::string out;
	out.reserve(input.size() * 3);

	bool capitalizeNext = true;

	for (size_t i = 0; i < input.size(); ++i) {
		char c = input[i];

		// Underscore → space
		if (c == '_') {
			out.push_back(' ');
			capitalizeNext = true;
			continue;
		}

		// CamelCase → add space before capital letters
		if (i > 0 && std::isupper(static_cast<unsigned char>(c))) {
			out.push_back(' ');
			capitalizeNext = true;
		}

		// Multi-digit number block
		if (std::isdigit(static_cast<unsigned char>(c))) {
			out.push_back(' ');

			// capture entire number run
			size_t start = i;
			while (i < input.size() && std::isdigit(static_cast<unsigned char>(input[i]))) {
				out.push_back(input[i]);
				i++;
			}

			out.append(" - ");
			capitalizeNext = true;

			i--; // offset the for-loop's increment
			continue;
		}

		// Normal letters
		if (capitalizeNext && std::isalpha(static_cast<unsigned char>(c))) {
			out.push_back(std::toupper(static_cast<unsigned char>(c)));
		}
		else {
			out.push_back(c);
		}

		capitalizeNext = false;
	}

	return out;
}

std::string helper_GetRoutineLabel(const Routine& routine)
{
	if (!routine.label.empty())
		return routine.label;
	
	return helper_FormatName(routine.name);
}

// Allows the user to edit the name of a routine (based on the wmID)
void EditRoutine(int wmId)
{
	// Get position of routine to be editable
	int routinePosition = wmId - EDIT_ROUTINE;

	// Find routine name at position i
	std::list<std::string>::iterator it = OrderTracker.routineNames.begin();
	for (int i = 0; i < routinePosition; i++)
		it++;
	std::string routineName = *it;

	// Find routineGUI with corresponding name
	auto routineGUI = Routines.find(routineName);
	if (routineGUI == Routines.end()) {
		OutputLogStr.append("> Error: Routine not found.\r\n");
		return;
	}

	// Allocate string on heap to pass to dialog (will be freed in dialog callback)
	std::string* pRoutineName = new std::string(routineName);
	
	// Show edit dialog
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDITROUTINE), gHWND, EditRoutineCB, reinterpret_cast<LPARAM>(pRoutineName));
	
	// Clean up allocated string
	delete pRoutineName;
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
			case ID_GENERATEDCODE_USECHRISTMASTRAINSETUP:
				Options.bUseChristmasTrainSetup = !Options.bUseChristmasTrainSetup;
				CheckMenuItem(GetMenu(hWnd), ID_GENERATEDCODE_USECHRISTMASTRAINSETUP, Options.bUseChristmasTrainSetup ? MF_CHECKED : MF_UNCHECKED);
				break;
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
				Options.wifiSSID = GetStringFromWindow(hWifiSSID);
				Options.wifiPassword = GetStringFromWindow(hWifiPass);
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
		using namespace UIConstants;
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		HBRUSH hBrush = CreateSolidBrush(COLOR_MY_BACKGROUND);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
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
		{
			// Set a font that supports emojis for the edit control
			HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
			HFONT hFont = CreateFontW(
				0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				L"Segoe UI"
			);
			if (hFont != NULL)
			{
				SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
			}
			
			code = GenerateCode();
			// Properly convert UTF-8 to UTF-16 for display
			std::wstring codeWide = UTF8ToUTF16(code);
			SetWindowTextW(hEdit, codeWide.c_str());
			return (INT_PTR)TRUE;
		}
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

// Message handler for Edit Routine dialog box.
INT_PTR CALLBACK EditRoutineCB(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
	{
		// lParam contains pointer to the routine name
		std::string* pRoutineName = reinterpret_cast<std::string*>(lParam);
		
		// Store the routine name pointer in window user data for later use
		SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pRoutineName));
		
		// Find routineGUI with corresponding name
		auto routineGUI = Routines.find(*pRoutineName);
		if (routineGUI != Routines.end())
		{
			// Get current label
			std::string currentLabel = helper_GetRoutineLabel(routineGUI->second.routine);
			
			// Set the edit control with current label
			HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
			std::wstring labelWide = UTF8ToUTF16(currentLabel);
			SetWindowTextW(hEdit, labelWide.c_str());
			
			// Set font
			HFONT hFont = CreateFontW(
				0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				L"Segoe UI"
			);
			if (hFont != NULL)
			{
				SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
			}
		}
		
		return (INT_PTR)TRUE;
	}
	
	switch (message)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDOK)
		{
			// Get the routine name from window user data
			std::string* pRoutineName = reinterpret_cast<std::string*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (pRoutineName != nullptr)
			{
				// Find routineGUI with corresponding name
				auto routineGUI = Routines.find(*pRoutineName);
				if (routineGUI != Routines.end())
				{
					// Get new label from edit control
					HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
					std::string newLabel = GetStringFromWindow(hEdit);
					
					// Trim whitespace
					newLabel = trim(newLabel);
					
					// Set the label (empty string means use formatted name)
					routineGUI->second.routine.label = newLabel;
					
					// Update the GUI
					DrawRoutineList();
					SetWindowTextW(hOutputLog, std::wstring(OutputLogStr.begin(), OutputLogStr.end()).c_str());
				}
			}
			
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


// Sets up the GUI for all the options and controls of this application
void AddControls(HWND handler)
{
	using namespace UIConstants;
	
	if (g_HeaderFont == NULL)
	{
		g_HeaderFont = CreateFont(FONT_SIZE_HEADER, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_FAMILY);
	}

	if (g_ButtonFont == NULL)
	{
		g_ButtonFont = CreateFont(FONT_SIZE_BUTTON, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_FAMILY);
	}
	if (g_OptionsHeaderFont == NULL)
	{
		g_OptionsHeaderFont = CreateFont(FONT_SIZE_SUBHEADER, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_FAMILY);
	}
	if (g_LabelFont == NULL)
	{
		g_LabelFont = CreateFont(FONT_SIZE_LABEL, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_FAMILY);
	}
	if (g_InputFont == NULL)
	{
		g_InputFont = CreateFont(FONT_SIZE_INPUT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_FAMILY);
	}

	RECT rect;
	int i;
	if (GetWindowRect(handler, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		RoutineWidth = width / 2 - (RoutineBtnWidth * NUM_ROUTINE_BUTTONS) - (MARGIN_XLARGE * 2);
	}

	hwdHandler = handler;

	int HeaderRowHeights[4];
	int OptionsRowHeight[7];

	HeaderRowHeights[0] = MARGIN_MEDIUM;
	OptionsRowHeight[0] = HeaderRowHeights[0] + MARGIN_XLARGE;
	OptionsRowHeight[1] = OptionsRowHeight[0];
	OptionsRowHeight[2] = OptionsRowHeight[1];
	HeaderRowHeights[1] = OptionsRowHeight[2] + (SECTION_SPACING + MARGIN_MEDIUM);
	OptionsRowHeight[3] = HeaderRowHeights[1] + (MARGIN_MEDIUM + MARGIN_SMALL);
	HeaderRowHeights[2] = OptionsRowHeight[3] + (SECTION_SPACING + MARGIN_MEDIUM);
	OptionsRowHeight[4] = HeaderRowHeights[2] + (MARGIN_MEDIUM + MARGIN_SMALL);
	OptionsRowHeight[5] = OptionsRowHeight[4] + (MARGIN_MEDIUM + MARGIN_SMALL);
	HeaderRowHeights[3] = OptionsRowHeight[5] + (SECTION_SPACING + MARGIN_MEDIUM);
	OptionsRowHeight[6] = HeaderRowHeights[3] + (MARGIN_MEDIUM + MARGIN_SMALL);


	int ColumSpace = COLUMN_SPACING;
	int ItemHeight = BUTTON_HEIGHT_STANDARD;
	int HeaderHeight = HEADER_HEIGHT;
	int LabelHeight = LABEL_HEIGHT;
	int InputHeight = INPUT_HEIGHT;

	int NextItemW = 0;
	int NextItemH = 0;


	int cbHeight = LABEL_HEIGHT;
	int secondColumnStart = RoutineWidth + (RoutineBtnWidth*NUM_ROUTINE_BUTTONS) + (MARGIN_XLARGE * 3);
	int reqFielsLen = 105;
	int pinEditWidth = INPUT_WIDTH_PIN;
	int thirdColumnStart = secondColumnStart + reqFielsLen + pinEditWidth + (MARGIN_MEDIUM * 3);
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
		MARGIN_SMALL,
		HeaderRowHeights[0] + HEADER_HEIGHT + BUTTON_HEIGHT_STANDARD + (MARGIN_MEDIUM * 2),
		RoutineWidth + (RoutineBtnWidth * NUM_ROUTINE_BUTTONS) + (MARGIN_MEDIUM * 3),
		height - 175,
		handler,
		NULL,
		NULL,
		NULL
	);

	// Routines
	NextItemH = HeaderRowHeights[0];
	HWND hStatic = CreateWindowW(L"Static", L"ROUTINES", WS_VISIBLE | WS_CHILD, MARGIN_SMALL, HeaderRowHeights[0], secondColumnStart - (MARGIN_MEDIUM * 3), HEADER_HEIGHT + MARGIN_MEDIUM, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_HeaderFont, TRUE);
	hStatic = CreateWindowW(L"Button", L"Add", WS_VISIBLE | WS_CHILD, MARGIN_SMALL, HeaderRowHeights[0] + HEADER_HEIGHT + MARGIN_SMALL, BUTTON_WIDTH_MEDIUM, BUTTON_HEIGHT_STANDARD, handler, (HMENU)CREATE_ROUTINE, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	hStatic = CreateWindowW(L"Button", L"Clear All", WS_VISIBLE | WS_CHILD, MARGIN_SMALL + BUTTON_WIDTH_MEDIUM + MARGIN_SMALL, HeaderRowHeights[0] + HEADER_HEIGHT + MARGIN_SMALL, BUTTON_WIDTH_LARGE, BUTTON_HEIGHT_STANDARD, handler, (HMENU)CLEAR_ROUTINES, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);

	// Options Header
	NextItemW = secondColumnStart;
	hStatic = CreateWindowW(L"Static", L"OPTIONS", WS_VISIBLE | WS_CHILD, NextItemW, HeaderRowHeights[0], secondColumnStart - (MARGIN_MEDIUM * 3), HEADER_HEIGHT + MARGIN_MEDIUM, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_HeaderFont, TRUE);
	
	// Train Options Group
	NextItemH = HeaderRowHeights[0] + HEADER_HEIGHT + (MARGIN_MEDIUM * 2);
	int trainGroupY = NextItemH;
	int trainGroupHeight = 80;
	HWND hTrainGroup = CreateWindowW(L"Button", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, secondColumnStart - MARGIN_SMALL, trainGroupY, width - secondColumnStart - MARGIN_MEDIUM, trainGroupHeight, handler, NULL, NULL, NULL);
	
	int HeaderCenter = secondColumnStart;
	hStatic = CreateWindowW(L"Static", L"TRAIN OPTIONS", WS_VISIBLE | WS_CHILD, HeaderCenter + MARGIN_SMALL, trainGroupY - (HEADER_HEIGHT / 2), 160, HEADER_HEIGHT, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	
	NextItemW = secondColumnStart + GROUP_BOX_PADDING;
	NextItemH = trainGroupY + GROUP_BOX_PADDING;
	hStatic = CreateWindowW(L"Static", L"Train Init Duration: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 125, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 125 + LABEL_INPUT_GAP;
	hTrainResetDuration = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, 55, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hTrainResetDuration, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	NextItemW += 55 + ColumSpace;
	hStatic = CreateWindowW(L"Static", L"Train Pin (Left to Right): ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 175, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 175 + LABEL_INPUT_GAP;
	hTrainLeftPin = CreateWindowW(L"Edit", L"A5", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hTrainLeftPin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	NextItemW += pinEditWidth + ColumSpace;
	hStatic = CreateWindowW(L"Static", L"Train Pin (Right to Left): ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 175, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 175 + LABEL_INPUT_GAP;
	hTrainRightPin = CreateWindowW(L"Edit", L"A4", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hTrainRightPin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);

	NextItemW = secondColumnStart + GROUP_BOX_PADDING;
	NextItemH += InputHeight + CONTROL_SPACING;
	hStatic = CreateWindowW(L"Static", L"Motor Voltage Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 125, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 125 + LABEL_INPUT_GAP;
	hTrainMotorPin = CreateWindowW(L"Edit", L"A6", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hTrainMotorPin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	
	// MP3 Options Group
	NextItemW = secondColumnStart;
	NextItemH = trainGroupY + trainGroupHeight + SECTION_SPACING;
	int mp3GroupY = NextItemH;
	int mp3GroupHeight = 60;
	HWND hMP3Group = CreateWindowW(L"Button", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, secondColumnStart - MARGIN_SMALL, mp3GroupY, width - secondColumnStart - MARGIN_MEDIUM, mp3GroupHeight, handler, NULL, NULL, NULL);
	
	hStatic = CreateWindowW(L"Static", L"MP3 OPTIONS", WS_VISIBLE | WS_CHILD, HeaderCenter + MARGIN_SMALL, mp3GroupY - (HEADER_HEIGHT / 2), 155, HEADER_HEIGHT, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	
	NextItemW = secondColumnStart + GROUP_BOX_PADDING;
	NextItemH = mp3GroupY + GROUP_BOX_PADDING;
	hStatic = CreateWindowW(L"Static", L"Power/Skip Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 105, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 105 + LABEL_INPUT_GAP;
	hMP3Pin = CreateWindowW(L"Edit", L"A3", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hMP3Pin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	NextItemW += pinEditWidth + ColumSpace;
	hStatic = CreateWindowW(L"Static", L"Volume Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 83, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 83 + LABEL_INPUT_GAP;
	hMP3VolPin = CreateWindowW(L"Edit", L"A2", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hMP3VolPin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	NextItemW += pinEditWidth + ColumSpace;
	hStatic = CreateWindowW(L"Static", L"Windows Drive Letter: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 150, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 150 + LABEL_INPUT_GAP;
	hMP3DriveLetter = CreateWindowW(L"Edit", L"D", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hMP3DriveLetter, WM_SETFONT, (WPARAM)g_InputFont, TRUE);

	// Misc Options Group
	NextItemW = secondColumnStart;
	NextItemH = mp3GroupY + mp3GroupHeight + SECTION_SPACING;
	int miscGroupY = NextItemH;
	int miscGroupHeight = 50;
	HWND hMiscGroup = CreateWindowW(L"Button", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, secondColumnStart - MARGIN_SMALL, miscGroupY, width - secondColumnStart - MARGIN_MEDIUM, miscGroupHeight, handler, NULL, NULL, NULL);
	
	hStatic = CreateWindowW(L"Static", L"MISC OPTIONS", WS_VISIBLE | WS_CHILD, HeaderCenter + MARGIN_SMALL, miscGroupY - (HEADER_HEIGHT / 2), 155, HEADER_HEIGHT, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	
	NextItemW = secondColumnStart + GROUP_BOX_PADDING;
	NextItemH = miscGroupY + GROUP_BOX_PADDING;
	hStatic = CreateWindowW(L"Static", L"Motion Sense Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 122, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 122 + LABEL_INPUT_GAP;
	hMotionSensorPin = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hMotionSensorPin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	NextItemW += pinEditWidth + ColumSpace;
	hStatic = CreateWindowW(L"Static", L"Randomize Seed Pin: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 145, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 145 + LABEL_INPUT_GAP;
	hRandomSeedPin = CreateWindowW(L"Edit", L"A7", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, pinEditWidth, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hRandomSeedPin, WM_SETFONT, (WPARAM)g_InputFont, TRUE);

	// ESP32 Options Group
	NextItemW = secondColumnStart;
	NextItemH = miscGroupY + miscGroupHeight + SECTION_SPACING;
	int esp32GroupY = NextItemH;
	int esp32GroupHeight = 50;
	HWND hESP32Group = CreateWindowW(L"Button", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, secondColumnStart - MARGIN_SMALL, esp32GroupY, width - secondColumnStart - MARGIN_MEDIUM, esp32GroupHeight, handler, NULL, NULL, NULL);
	
	hStatic = CreateWindowW(L"Static", L"ESP32 OPTIONS (leave blank if not using ESP32 board)", WS_VISIBLE | WS_CHILD, HeaderCenter + MARGIN_SMALL, esp32GroupY - (HEADER_HEIGHT / 2), 435, HEADER_HEIGHT, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	
	NextItemW = secondColumnStart + GROUP_BOX_PADDING;
	NextItemH = esp32GroupY + GROUP_BOX_PADDING;
	hStatic = CreateWindowW(L"Static", L"WiFi SSID: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 100, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 100 + LABEL_INPUT_GAP;
	hWifiSSID = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, INPUT_WIDTH_STANDARD, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hWifiSSID, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	NextItemW += INPUT_WIDTH_STANDARD + ColumSpace;
	hStatic = CreateWindowW(L"Static", L"WiFi Pass: ", WS_VISIBLE | WS_CHILD, NextItemW, NextItemH, 100, LabelHeight, handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
	NextItemW += 100 + LABEL_INPUT_GAP;
	hWifiPass = CreateWindowW(L"Edit", L"", WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_AUTOHSCROLL | WS_BORDER, NextItemW, NextItemH, INPUT_WIDTH_STANDARD, InputHeight, handler, NULL, NULL, NULL);
	SendMessage(hWifiPass, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	if (bFirstDraw)
	{
		bFirstDraw = false;
		// Set the text of the password to what is stored in Options.wifiPassword
		SetWindowTextW(hWifiPass, std::wstring(Options.wifiPassword.begin(), Options.wifiPassword.end()).c_str());
		// Set the text of the ssid to what is stored in Options.wifiSSID
		SetWindowTextW(hWifiSSID, std::wstring(Options.wifiSSID.begin(), Options.wifiSSID.end()).c_str());
	}


	// Bottom Bar
	int bottomStart = esp32GroupY + esp32GroupHeight + (SECTION_SPACING * 2);
	int bottomColumStart = secondColumnStart;
	int outputLength = width - secondColumnStart - MARGIN_MEDIUM;
	
	hStatic = CreateWindowW(L"Button", L"View Generated Code", WS_VISIBLE | WS_CHILD, 
		bottomColumStart,
		bottomStart, 
		BUTTON_WIDTH_XLARGE, 
		BUTTON_HEIGHT_LARGE, 
		handler, (HMENU)GENERATE_CODE, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	
	hStatic = CreateWindowW(L"Button", L"Create .ino File", WS_VISIBLE | WS_CHILD, 
		bottomColumStart + BUTTON_WIDTH_XLARGE + MARGIN_SMALL,
		bottomStart, 
		BUTTON_WIDTH_XLARGE, 
		BUTTON_HEIGHT_LARGE, 
		handler, (HMENU)WRITE_TO_ARDUINO, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	
	hStatic = CreateWindowW(L"Button", L"Upload to Device(s)", WS_VISIBLE | WS_CHILD, 
		bottomColumStart + (BUTTON_WIDTH_XLARGE * 2) + (MARGIN_SMALL * 2),
		bottomStart, 
		BUTTON_WIDTH_XLARGE, 
		BUTTON_HEIGHT_LARGE, 
		handler, (HMENU)UPLOAD_CODE, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
		
	hStatic = CreateWindowW(L"Static", L"Output Log: ", WS_VISIBLE | WS_CHILD, 
		bottomColumStart,
		bottomStart + BUTTON_HEIGHT_LARGE + MARGIN_SMALL, 
		outputLength,
		HEADER_HEIGHT, 
		handler, NULL, NULL, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)g_OptionsHeaderFont, TRUE);
	
	hClearLog = CreateWindowW(L"Button", L"Clear  (X)", WS_VISIBLE | WS_CHILD, 
		bottomColumStart + 100,
		bottomStart + BUTTON_HEIGHT_LARGE + MARGIN_SMALL,
		BUTTON_WIDTH_SMALL,
		LABEL_HEIGHT,
		handler, (HMENU)CLEAR_LOG, NULL, NULL);
	SendMessage(hClearLog, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
	
	hOutputLog = CreateWindowW(L"Edit", L"", 
		WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | 
		ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		bottomColumStart,
		bottomStart + BUTTON_HEIGHT_LARGE + HEADER_HEIGHT + (MARGIN_SMALL * 2), 
		outputLength, 
		height - (bottomStart + BUTTON_HEIGHT_LARGE + HEADER_HEIGHT + (MARGIN_MEDIUM * 3)),
		handler, NULL, NULL, NULL);
	SendMessage(hOutputLog, WM_SETFONT, (WPARAM)g_InputFont, TRUE);
	
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

		// Create string containing the routine name, number of lights, and label
		std::ostringstream osRoutineStr;
		osRoutineStr << ++i << ") ";
		if (!routineGUI->routine.label.empty()) {
			osRoutineStr << "\"" << routineGUI->routine.label << "\" ";
		} else {
			osRoutineStr << routineGUI->routine.name << " ";
		}
		osRoutineStr << "(" << routineGUI->routine.lights.size() << " lights)\r\n";
		std::string tempStr = osRoutineStr.str();
		std::wstring wStr = std::wstring(tempStr.begin(), tempStr.end());

		// Create windows for the routine name, and buttons to change the routine's position in the list
		int rowHeight = RoutineHeight;
		int buttonYPos = currentYPos;
		routineGUI->title = CreateWindowW(L"Static", wStr.c_str(), WS_VISIBLE | WS_CHILD, 
			UIConstants::MARGIN_SMALL, 
			currentYPos, 
			RoutineWidth, 
			rowHeight,
			hRoutineScrollContainer, NULL, NULL, NULL);
		SendMessage(routineGUI->title, WM_SETFONT, (WPARAM)g_LabelFont, TRUE);
		routineGUI->editButton = CreateWindowW(L"Button", L"Edit Label", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + UIConstants::MARGIN_MEDIUM + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth, 
			RoutineBtnHeight, 
			hRoutineScrollContainer, (HMENU)(EDIT_ROUTINE + i - 1), NULL, NULL);
		SendMessage(routineGUI->editButton, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
		routineGUI->upButton = CreateWindowW(L"Button", L"Move Up", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + UIConstants::MARGIN_MEDIUM + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth, 
			RoutineBtnHeight, 
			hRoutineScrollContainer, (HMENU)(MOVE_ROUTINE_UP + i - 1), NULL, NULL);
		SendMessage(routineGUI->upButton, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
		routineGUI->downButton = CreateWindowW(L"Button", L"Move Down", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + UIConstants::MARGIN_MEDIUM + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth, RoutineBtnHeight, 
			hRoutineScrollContainer, (HMENU)(MOVE_ROUTINE_DOWN + i - 1), NULL, NULL);
		SendMessage(routineGUI->downButton, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);
		//routineGUI->editButton = CreateWindowW(L"Button", L"e", WS_VISIBLE | WS_CHILD, RoutineWidth + 10 + (RoutineBtnWidth * buttonPos++), 45 + (RoutineHeight * i) + (i * 5), RoutineBtnWidth, RoutineBtnHeight, hwdHandler, (HMENU)(EDIT_ROUTINE + i - 1), NULL, NULL);
		routineGUI->deleteButton = CreateWindowW(L"Button", L"X", WS_VISIBLE | WS_CHILD, 
			RoutineWidth + UIConstants::MARGIN_MEDIUM + (RoutineBtnWidth * buttonPos++), 
			buttonYPos,
			RoutineBtnWidth/2, 
			RoutineBtnHeight,
			hRoutineScrollContainer, (HMENU)(DELETE_ROUTINE + i - 1), NULL, NULL);
		SendMessage(routineGUI->deleteButton, WM_SETFONT, (WPARAM)g_ButtonFont, TRUE);

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
	size_t pos1 = fileName.find_last_of("\\");
	size_t pos2 = fileName.rfind(".wav");
	if (pos1 == std::string::npos) pos1 = -1;
	if (pos2 == std::string::npos) pos2 = fileName.length();
	routineGUI.routine.label = fileName.substr(pos1 + 1, pos2 - (pos1 + 1));

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
	CodeGenerator cg(Options, bchA, bchD, GetBoolFromPinStr(Options.motionSensorPin), Routines, OrderTracker);

	return cg.GenerateCode();
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

