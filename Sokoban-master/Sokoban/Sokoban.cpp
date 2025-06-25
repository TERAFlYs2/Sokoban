#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <direct.h>
#include "resource.h"
using namespace std;

#pragma comment(lib,"winmm.lib")

#define MAXSTAGE 20 // ���������� ��������
#define BW 32 // ������ ���������� �����������, ������� ����� �������������� ��� ������
#define BH 32 // ������ ���������� �����������, ������� ����� �������������� ��� ������
#define MAXUNDO 1000 // ������������ ����������, ������� ����� ������� ���������� �� ������ �����������
#define LOAD_LEN 21
#define MAXSTRING 512 // ������������ �����, ������� ����� ��������� � ������� ��������
#define NOTICE TEXT("����������, ������� ��� ����� (������ �� ���������� �����).txt.")

void DrawScreen(HDC hdc);
BOOL TestEnd();
void Move(int dir);
void InitStage();
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);
void ErasePack(int x, int y);
void Undo();
void Redo();
void Save();
void Load();
void MakeMapList(TCHAR* MapName);
BOOL IsDuplicated(TCHAR* FileName);
BOOL MapLoader(char* FileName);
void SortRecords(int record[]);
void WriteRecords();
void ReadRecords();
void AlreadyDoneCheck();
BOOL AddStage(char* FileName);
void LoadAddStage();
void Remove(char* Name, char* rm);
char ns[18][21];
int nStage;
int nMaxStage = 3;
int nx, ny;
int nMove;
HBITMAP hBit[9];
int ManBit;
char arStage[MAXSTAGE][18][21] = {
	{
		"####################",
		"####################",
		"####################",
		"#####   ############",
		"#####O  ############",
		"#####  O############",
		"###  O O ###########",
		"### # ## ###########",
		"#   # ## #####  ..##",
		"# O  O   @      ..##",
		"##### ### # ##  ..##",
		"#####      #########",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################"
	},
	{
		"####################",
		"####################",
		"####################",
		"####################",
		"####..  #     ######",
		"####..  # O  O  ####",
		"####..  #O####  ####",
		"####..    @ ##  ####",
		"####..  # #  O #####",
		"######### ##O O ####",
		"###### O  O O O ####",
		"######    #     ####",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################"
	},
	{
		"####################",
		"####################",
		"####################",
		"####################",
		"##########     @####",
		"########## O#O #####",
		"########## O  O#####",
		"###########O O #####",
		"########## O # #####",
		"##....  ## O  O  ###",
		"###...    O  O   ###",
		"##....  ############",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################",
		"####################"
	}
};
char nCustomStage[18][21];
char TempStage[18][21];
BOOL bSound = TRUE;
TCHAR CMapName[64];
char NameToChar[64];
BOOL CustomFlag = FALSE;
int Records[MAXSTAGE][5];
int RecordsSize[MAXSTAGE];
BOOL errflag = FALSE;

typedef struct tag_MoveInfo
{
	char dx : 3;
	char dy : 3;
	char bWithPack : 2;
} MoveInfo;

MoveInfo MOVEINFO[MAXUNDO];
int UndoIdx;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MapDlgProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain, hMDlg, hMapDlg;
LPCTSTR lpszClass = TEXT("Sokoban");

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);
	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	while (GetMessage(&Message, NULL, 0, 0))
	{
		if ((!IsWindow(hMDlg) || !IsDialogMessage(hMDlg, &Message)) || (!IsDialogMessage(hMapDlg, &Message)))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	Save();
	WriteRecords();
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	int i;
	TCHAR Message[256];
	switch (iMessage)
	{
	case WM_CREATE:
		hWndMain = hWnd;
		SetRect(&crt, 0, 0, 900, BH * 18.6);
		AdjustWindowRect(&crt, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
		SetWindowPos(hWnd, NULL, 0, 0, crt.right - crt.left, crt.bottom - crt.top, SWP_NOMOVE | SWP_NOZORDER);
		for (i = 0; i < 9; i++)
		{
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_WALL + i));
		}
		//nStage = 0;
		Load();
		ReadRecords();
		if (errflag)
		{
			for (int i = 3; i < nMaxStage; i++)
			{
				memset(Records[i], 0, sizeof(int) * 5);
				RecordsSize[i] = 0;
			}
			WriteRecords();
		}
		InitStage();
		/*if (CustomFlag)
		{
			if (!MapLoader(NameToChar))
			{
				MessageBox(HWND_DESKTOP, TEXT("��� ����������� ���������������� ����. ���������������� �� ����� 1."), TEXT("�����������"), MB_OK);
				nStage = 0;
				CustomFlag = FALSE;
				InitStage();
			}
		}*/
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawScreen(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			Move(wParam);
			if (TestEnd())
			{
				if (bSound)
				{
					PlaySound(MAKEINTRESOURCE(IDR_CLEAR), g_hInst, SND_RESOURCE | SND_ASYNC);
				}
				if (nStage < nMaxStage)
				{
					wsprintf(Message, TEXT("%d � ������������� �����. ������� � ���������� �����."), nStage + 1);
				}
				MessageBox(hWnd, (LPWSTR)Message, TEXT("�����������"), MB_OK);
				if (nStage < nMaxStage - 1)
				{
					nStage++;
				}
				InitStage();
			}
			break;
		case 'Q':
			DestroyWindow(hWnd);
			break;
		case 'R':
			InitStage();
			break;
		case 'N':
			if (nStage < nMaxStage - 1)
			{
				nStage++;
				InitStage();
			}
			break;
		case 'P':
			CustomFlag = FALSE;
			if (nStage > 0)
			{
				nStage--;
				InitStage();
			}
			break;
		case 'Z':
			Undo();
			break;
		case 'Y':
			Redo();
			break;
		case 'S':
			bSound = !bSound;
			break;
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_LOADCUSTOM: // ID �������� ���������������� ����� �� ����
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_MAKEMAP), hWnd, MapDlgProc) == IDOK)
			{ // IDD_MAKEMAP � ��� ������������� ����������������� ����������� ���� ������� ����.
				memset(NameToChar, 0, sizeof(char) * 64);
				WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), NameToChar, 64, 0, 0); // �������������� ���� ������ TCHAR* � ��� ������ char*
				MapLoader(NameToChar); // ��������� ���� �����, ��������������� �������� �����.
				CustomFlag = TRUE; // ����, �����������, �������������� �� �������� ���������������� �����
				if (AddStage(NameToChar)) // �������� ����������� ����� � �������� ���������� ����� ����� ���������� ����� ������� ����.
				{
					InitStage(); // ����� ������ � ����������� ������
				}
			}
			break;
		case ID_HELP:
			if (!IsWindow(hMDlg))
			{
				hMDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG), hWnd, DlgProc);
				ShowWindow(hMDlg, SW_SHOW);
			}
			break;
		case ID_INFO:
			MessageBox(hWnd, TEXT("���������: �������� �����\nE-mail: danylo.reznikov@nure.ua"), TEXT("����������"), MB_OK);
			break;
		case ID_CLOSE:
			DestroyWindow(hWnd);
			break;
		}
		return 0;
	case WM_DESTROY:
		for (i = 0; i < 9; i++)
		{
			DeleteObject(hBit[i]);
		}
		PostQuitMessage(0);
		return 0;
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{ // ���������, ������� ������������ ��������� �� ���������� ���� �������. �����������
	TCHAR str[MAXSTRING];
	switch (iMessage)
	{
	case WM_INITDIALOG:
		LoadString(g_hInst, IDS_SAMPLE, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_SAMPLE, str);
		LoadString(g_hInst, IDS_HOWTO, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_HOWTO, str);
		LoadString(g_hInst, IDS_EMPTY, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_EMPTY, str);
		LoadString(g_hInst, IDS_STUFF, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_STUFF, str);
		LoadString(g_hInst, IDS_GOAL, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_GOAL, str);
		LoadString(g_hInst, IDS_CHARACTER, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_CHARACTER, str);
		LoadString(g_hInst, IDS_WALL, str, MAXSTRING);
		SetDlgItemText(hDlg, IDC_WALL, str);
		return TRUE; // �������� ������� ����� � ��������� ������ � ������ �������������.
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		case IDOK:
			DestroyWindow(hDlg);
			EndDialog(hDlg, IDOK);
			hMDlg = NULL;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK MapDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{ // ��������� ��������� ��������� �� ����������� ���� ���������� ���������������� �����. ���������
	FILE* f;
	FILE* IsF;
	TCHAR FileName[64];
	char ExistFile[64];
	int i = 0;
	switch (iMessage)
	{
	case WM_INITDIALOG:
		hMapDlg = hDlg;
		fopen_s(&f, "MapList.txt", "r"); // MapList.txt � ��� ����, ���������� ������ ���� ����������� ����.
		if (f != NULL)                   // �������� ������ ����� ������������ � MapList.txt
		{
			while (TRUE)
			{
				memset(ExistFile, 0, sizeof(char) * 64);
				fscanf_s(f, "%ls", FileName, 64);
				if (feof(f)) break;
				WideCharToMultiByte(CP_ACP, 0, FileName, lstrlen(FileName), ExistFile, 64, 0, 0); // �������������� ���� ������ TCHAR* � ��� ������ char*
				fopen_s(&IsF, ExistFile, "r"); // ���������� ���� �� ���� ���� � �������� ���� ������ ��� ������, ����� ���������, ���������� �� ���� �����. 
				if (IsF != NULL)
				{
					SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_ADDSTRING, i, (LPARAM)FileName); // ���� ���� ����� ����������, �������� ��� � ������
					i++;
					fclose(IsF);
				}
			}
			fclose(f);
		}
		SetDlgItemText(hDlg, IDC_LOADMAP, NOTICE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CMAPLIST: // ID ������
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				i = SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETCURSEL, 0, 0); // ��������� ����� ������ � ���� ������, � ������� ��������� ��������� ������.
				SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETTEXT, i, (LPARAM)CMapName); // ��������� ������ � i-� ������ � CMapName
				SetDlgItemText(hDlg, IDC_LOADMAP, CMapName); // IDC_LOADMAP � ��� ������������� ��������������. ������� ������ ���������� ������ ��� ��������������
				return TRUE;
			}
			return FALSE;
		case IDOK:
			GetDlgItemText(hDlg, IDC_LOADMAP, CMapName, 64); // ��������� ������ � ��������������
			if (lstrcmp(CMapName, NOTICE) == 0)
			{ // ���� ����������� ������ ��������� �� �������, ������������� �� ���������, ����������� ���� ��������� ����������
				MessageBox(hMapDlg, TEXT("����������, ������� ���������� �������� �����."), TEXT("�����������"), MB_OK);
				return TRUE;
			}
			memset(ExistFile, 0, sizeof(char) * 64);
			WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), ExistFile, 64, 0, 0);
			fopen_s(&f, ExistFile, "r");
			if (f == NULL)
			{ // �������� ��� ����� ������ ��� ������, ����� ��������� �� �������������. ���� ���, ����������� ���� ���������, ����� ���������
				MessageBox(hMapDlg, TEXT("��� �����."), TEXT("�����������"), MB_OK);
				return TRUE;
			}
			if (!IsDuplicated(CMapName)) // ��������� ������� ���������� ������ � ���� �� ����� �����
			{
				MakeMapList(CMapName); // ���� ������������� ���� ���, �� ������������ � MapList.txt
			}
			fclose(f);
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void DrawScreen(HDC hdc)
{
	int x, y;
	int iBit;
	TCHAR Message[256];
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			switch (ns[y][x])
			{
			case '#':
				iBit = 0;
				break;
			case 'O':
				iBit = 1;
				break;
			case '.':
				iBit = 2;
				break;
			case ' ':
				iBit = 3;
				break;
			case '!':
				iBit = 8;
				break;
			}
			DrawBitmap(hdc, x * BW, y * BH, hBit[iBit]);
		}
	}
	DrawBitmap(hdc, nx * BW, ny * BH, hBit[ManBit]);
	wsprintf(Message, TEXT("Sokoban"));
	TextOut(hdc, 700, 10, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Q:�����, R:����������"));
	TextOut(hdc, 700, 30, Message, lstrlen(Message));
	wsprintf(Message, TEXT("N:���������, P:����������"));
	TextOut(hdc, 700, 50, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Z:������, Y:������������"));
	TextOut(hdc, 700, 70, Message, lstrlen(Message));
	wsprintf(Message, TEXT("S:���� ���/����"));
	TextOut(hdc, 700, 90, Message, lstrlen(Message));
	if (nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("���� : %d"), nStage + 1);
	}
	TextOut(hdc, 700, 120, Message, lstrlen(Message));
	wsprintf(Message, TEXT("���������� ����� : %d"), nMove);
	TextOut(hdc, 700, 140, Message, lstrlen(Message));
	if (RecordsSize[nStage] != 0 && nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("��������� ����"));
		TextOut(hdc, 700, 180, Message, lstrlen(Message));
		for (int i = 0; i < RecordsSize[nStage]; i++)
		{
			wsprintf(Message, TEXT("����: %d"), i + 1, Records[nStage][i]);
			TextOut(hdc, 700, 180 + 20 * (i + 1), Message, lstrlen(Message));
		}
	}
}

BOOL TestEnd()
{ // �������, ������� ���������, ����������� �� ����� �� ���� ������� ������.
	int x, y;
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (TempStage[y][x] == '.' && ns[y][x] != '!') //  ����� ����� ����������� �� �� ���� ������� �������� 
			{
				return FALSE;
			}
		}
	}
	/*if (nStage < nMaxStage)
	{
		SortRecords(Records[nStage]);
	}*/
	SortRecords(Records[nStage]); // ������� ���������� ������� �������� �����
	return TRUE;
}

void Move(int dir)
{
	int dx = 0, dy = 0; // ����������, ������������ ���������� � ����������� ��������
	BOOL bWithPack = FALSE; // ���� ��� �������� �����, ������������, ����� �������� �������� � ��������, � ����� �������� ���������� ����.
	BOOL Success = FALSE; // ����, ����� �������� ����, ����� ������� ���������� � ������� ������� 
	int i;
	switch (dir)
	{
	case VK_LEFT:
		ManBit = 4; // ManBit � ��������� ����������� ���������. ������� ������ ��������� ����������� ��� ��������, �������, ������ � �������
		dx = -1;
		break;
	case VK_RIGHT:
		ManBit = 5;
		dx = 1;
		break;
	case VK_UP:
		ManBit = 6;
		dy = -1;
		break;
	case VK_DOWN:
		ManBit = 7;
		dy = 1;
		break;
	}
	if (ns[ny + dy][nx + dx] != '#') // ns � ��� ������, � ������� �������� ������ �������� �����, ����������� ��� ������ ����.
	{ // ����� ��������� ������� �� �����
		if (ns[ny + dy][nx + dx] == 'O' || ns[ny + dy][nx + dx] == '!')
		{ // ����� ��������� ������������ �������� ������, ������������� � ������� �������
			if (ns[ny + dy * 2][nx + dx * 2] == ' ' || ns[ny + dy * 2][nx + dx * 2] == '.')
			{ // ����� ����� ��������� ������� �������� ������ ������ ��� ������� ��������
				ErasePack(nx + dx, ny + dy); // �������, ������� ���������� ������� ������ � ����������� �������� � ��������� ������ . ��� ������ � �����������, �������� � �������� ���������.
				bWithPack = TRUE;
				if (ns[ny + dy * 2][nx + dx * 2] == '.')
				{ // ����� ��������� ������� �������� ������� ��������
					Success = TRUE;
					ns[ny + dy * 2][nx + dx * 2] = '!'; // ��������� �����, �������������� ��������, ������������� � ������� ���������
				}
				else if (ns[ny + dy * 2][nx + dx * 2] == ' ')
				{ // ����� ��������� ����� �����
					ns[ny + dy * 2][nx + dx * 2] = 'O'; // ��������� ������, �������������� ��������
				}
			}
			else
			{
				return;
			}
		}
		nx += dx;
		ny += dy;
		nMove++; // ���������� ���������� �����
		MOVEINFO[UndoIdx].dx = dx; // 
		MOVEINFO[UndoIdx].dy = dy; //
		MOVEINFO[UndoIdx].bWithPack = bWithPack; // ������� ���������� � ������� ��������, � ������� �������� ���������� �� ������/���������� �����������.
		UndoIdx++;
		MOVEINFO[UndoIdx].dx = -2; // ����� -2, ���������� ����� � �������, ��� �������� ����������, �� ����� � �������, ������� ��� �� ���������.
		if (UndoIdx == MAXUNDO - 1)
		{
			for (i = 100; i < UndoIdx; i++)
			{
				MOVEINFO[i - 100] = MOVEINFO[i];
			}
			for (i = MAXUNDO - 100; i < MAXUNDO; i++)
			{
				MOVEINFO[i].dx = -2;
			}
			UndoIdx -= 100;
		}
		if (bSound)
		{
			PlaySound(MAKEINTRESOURCE(bWithPack ? IDR_WITHPACK : IDR_MOVE), g_hInst, SND_RESOURCE | SND_ASYNC);
		}
		if (bSound && Success)
		{
			PlaySound(MAKEINTRESOURCE(IDR_SUCCESS), g_hInst, SND_RESOURCE | SND_ASYNC);
		}
		InvalidateRect(hWndMain, NULL, FALSE);
	}
}

void InitStage()
{
	int x, y;
	ManBit = 4;
	if (CustomFlag)
	{ // ��� ������������� ���������������� �������� �����
		/*memcpy(TempStage, nCustomStage, sizeof(TempStage));
		AlreadyDoneCheck();
		memcpy(ns, nCustomStage, sizeof(ns));*/
		nStage = nMaxStage - 1; // ����������� ����� ���������� ����������� ������
	}
	memcpy(TempStage, arStage[nStage], sizeof(TempStage)); // ������ arStage � ��� ���������� ������, � ������� �������� ��� ����� ����.
	AlreadyDoneCheck(); // �������, ������� ����������� ������ ! � ������ . � ��������� ��� � ������� �������� ����� ��� ���������.
	memcpy(ns, arStage[nStage], sizeof(ns));
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (ns[y][x] == '@')
			{ // ������� ���������� ��������� � ��������� �� � ������� ���������� ���������
				nx = x;
				ny = y;
				ns[y][x] = ' ';
			}
		}
	}
	nMove = 0;
	UndoIdx = 0;
	for (x = 0; x < MAXUNDO; x++)
	{
		MOVEINFO[x].dx = -2;
	}
	InvalidateRect(hWndMain, NULL, TRUE);
}

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;
	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);
	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;
	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);
	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}

void ErasePack(int x, int y)
{
	/*if ((
[nStage][y][x] == '.' || arStage[nStage][y][x] == '!') && !CustomFlag)
	{
		ns[y][x] = '.';
	}
	if ((nCustomStage[y][x] == '.' || nCustomStage[y][x] == '!') && CustomFlag)
	{
		ns[y][x] = '.';
	}
	if ((arStage[nStage][y][x] != '.' && arStage[nStage][y][x] != '!') && !CustomFlag)
	{
		ns[y][x] = ' ';
	}
	if ((nCustomStage[y][x] != '.' && nCustomStage[y][x] != '!') && CustomFlag)
	{
		ns[y][x] = ' ';
	}*/
	if (TempStage[y][x] == '.')
	{ // ���� �� ���������� (x, y) ���� �������� ������� ��������
		ns[y][x] = '.';
	}
	else
	{ // ���� ���������� (x, y) ���������� ���� ������ �������������
		ns[y][x] = ' ';
	}
}

void Undo()
{
	if (UndoIdx != 0)
	{
		UndoIdx--;
		if (MOVEINFO[UndoIdx].bWithPack)
		{
			ErasePack(nx + MOVEINFO[UndoIdx].dx, ny + MOVEINFO[UndoIdx].dy);
			if (TempStage[ny][nx] == '.')
			{
				ns[ny][nx] = '!';
			}
			else
			{
				ns[ny][nx] = 'O';
			}
		}
		nx -= MOVEINFO[UndoIdx].dx;
		ny -= MOVEINFO[UndoIdx].dy;
		InvalidateRect(hWndMain, NULL, FALSE);
	}
}

void Redo()
{
	if (MOVEINFO[UndoIdx].dx != -2)
	{
		nx += MOVEINFO[UndoIdx].dx;
		ny += MOVEINFO[UndoIdx].dy;
		if (MOVEINFO[UndoIdx].bWithPack)
		{
			ErasePack(nx, ny);
			if (TempStage[ny + MOVEINFO[UndoIdx].dy][nx + MOVEINFO[UndoIdx].dx] == '.')
			{
				ns[ny + MOVEINFO[UndoIdx].dy][nx + MOVEINFO[UndoIdx].dx] = '!';
			}
			else
			{
				ns[ny + MOVEINFO[UndoIdx].dy][nx + MOVEINFO[UndoIdx].dx] = 'O';
			}
		}
		InvalidateRect(hWndMain, NULL, FALSE);
		UndoIdx++;
	}
}

void Save()
{
	FILE* f;
	fopen_s(&f, "Save.dat", "wb");
	if (f != NULL)
	{
		for (int i = 0; i < 18; i++)
		{
			fprintf(f, "%s", ns[i]); // ��������� ������� ������ �����
		}
		fprintf(f, "%d", nStage); // ��������� ������� ����
		/*if (CustomFlag)
		{
			fprintf(f, "%s", NameToChar);
		}*/
		fclose(f);
	}
}

void Load()
{
	FILE* f;
	FILE* fr;
	fopen_s(&f, "Save.dat", "rb");
	if (f != NULL)
	{
		for (int i = 0; i < 18; i++)
		{
			fgets(ns[i], LOAD_LEN, f);
		}
		fscanf_s(f, "%d", &nStage);
		/*if (CustomFlag)
		{
			fgets(NameToChar, 64, f);
		}*/
		fclose(f);
	} // ��������� �����, ���������� � ������� ns
	/*if (CustomFlag)
	{
		MapLoader(NameToChar);
	}*/
	LoadAddStage(); // ����������� ����� �����������
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt � ��� ����, ���������� ����� ����������� � ������ ������ ����.
	char tempstr[64];
	memset(tempstr, 0, sizeof(char) * 64);
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", tempstr, 64);
			fopen_s(&fr, tempstr, "r"); // ���������� ���� ��� ����� � �������� ���� ������ ��� ������
			if (fr == NULL || nStage >= nMaxStage)
			{
				if (fr == NULL)
				{ // ���� ������ �� �������� � AddList.txt
					fclose(f);
					remove("AddList.txt"); // ������� ���� AddList.txt.
					MessageBox(HWND_DESKTOP, TEXT("��������� ������ ��� �������� ����������� �����. ���������������� �� ����� 1."), TEXT("�����������"), MB_OK);
				}
				if (nStage >= nMaxStage)
				{ // ���� ������� ����������� ����� ����� ������, ��� ���������� ����, ������� ���� � ���� �� ������ ������
					MessageBox(HWND_DESKTOP, TEXT("��������� ������. ��������������� �����."), TEXT("�����������"), MB_OK);
				}
				errflag = TRUE; // ����, �����������, ��� ��������� ������. ���������������� ������ (������) ����������� �����
				CustomFlag = FALSE;
				nStage = 0; // ���������������� �� ����� 1.
				break;
			}
			if (fr != NULL) fclose(fr);
			if (feof(f)) break;
		}
		if (f != NULL) fclose(f);
	}
}

void MakeMapList(TCHAR* MapName)
{ // �������, ������� ��������� ����������� ��� ����� �����
	FILE* f;
	fopen_s(&f, "MapList.txt", "a");
	if (f != NULL)
	{
		fprintf(f, "%ls\n", MapName);
		fclose(f);
	}
}

BOOL IsDuplicated(TCHAR* FileName)
{ // �������, ������� ���������, �� ����������� �� ��� ����� �����, �������� � �������� ���������.
	FILE* f;
	TCHAR name[64];
	fopen_s(&f, "MapList.txt", "r");
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%ls", name, 64);
			if (feof(f)) break;
			if (lstrcmp(name, FileName) == 0)
			{
				return TRUE;
			}
		}
		fclose(f);
	}
	return FALSE;
} // ���������� TRUE, ���� �����������, ���������� FALSE, ���� �� �����������.

BOOL MapLoader(char* FileName)
{ // �������, ������� ������ �����, ���������� � ����� ����� � ������ ����� �����, ��������� � �������� ���������.
	FILE* f;
	int i = 0;
	fopen_s(&f, FileName, "r");
	if (f == NULL) return FALSE;
	else
	{
		while (TRUE)
		{
			fread_s(nCustomStage[i], sizeof(nCustomStage[i]), sizeof(char), 21, f); // ������ Custom Stage � ��� ������, ������� ��������� � ������ ���������������� �����.
			nCustomStage[i][20] = 0;
			if (feof(f)) break;
			i++;
		}
		fclose(f);
		return TRUE;
	}
}

void SortRecords(int record[])
{ // ������� ���������� �������. ����������� ���������� ����� ����� ������������, ��������� ��� ��������� ��������� ���������� �������.
	BOOL flag = FALSE; // ����, �����������, �������� �� ������, �������� ������
	for (int i = 0; i < 5; i++)
	{
		if (record[i] == 0)
		{
			record[i] = nMove;
			RecordsSize[nStage]++; // RecordsSize � ��� ��������� ������, � ������� �������� ������ ��� ������� �����. ������ ������� ������� ����� ����� ������������ ����� 5
			flag = TRUE;
			break;
		}
	}
	if (flag) // ���� � ������� �������� ������ �����, ���������� ��������� �� ���������� ����������� �������
	{
		for (int i = 1; i < RecordsSize[nStage]; i++)
		{
			for (int j = 0; j < RecordsSize[nStage] - i; j++)
			{
				if (record[j] > record[j + 1])
				{
					int temp = record[j];
					record[j] = record[j + 1];
					record[j + 1] = temp;
				}
			}
		}
	}
	else
	{ // ���� � ������� �� �������� ������� �����, �������� ����� ������ ������ �, ���� ��� �����, ��������� ������� ������ � ����� ������ ������� ������ � ��������� ����������� ����������.
		if (record[4] < nMove)
		{
			return;
		}
		record[4] = nMove;
		for (int i = 1; i < RecordsSize[nStage]; i++)
		{
			for (int j = 0; j < RecordsSize[nStage] - i; j++)
			{
				if (record[j] > record[j + 1])
				{
					int temp = record[j];
					record[j] = record[j + 1];
					record[j + 1] = temp;
				}
			}
		}
	}
}

void WriteRecords()
{ // �������, ������������ ������ ��� ������� ����� � �������� ����
	FILE* f;
	fopen_s(&f, "Records.dat", "wb");
	if (f != NULL)
	{
		for (int i = 0; i < nMaxStage; i++)
		{
			fprintf(f, "%d ", RecordsSize[i]);
			for (int j = 0; j < 5; j++)
			{
				fprintf(f, "%d ", Records[i][j]);
			}
		}
		fclose(f);
	}
}

void ReadRecords()
{ // �������, ������� ������ ������, ���������� � ����, � ��������� �� � ������� �������.
	FILE* f;
	fopen_s(&f, "Records.dat", "rb");
	if (f != NULL)
	{
		for (int i = 0; i < nMaxStage; i++)
		{
			fscanf_s(f, "%d ", &RecordsSize[i]);
			for (int j = 0; j < 5; j++)
			{
				fscanf_s(f, "%d ", &Records[i][j]);
			}
		}
		fclose(f);
	}
}

void AlreadyDoneCheck()
{ // �������, ������� ����������� ������� ! � ������� . � ��������� ��.
	for (int i = 0; i < 18; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			TempStage[i][j] = (TempStage[i][j] == '!') ? '.' : TempStage[i][j];
		}
	}
}

BOOL AddStage(char* FileName)
{ // �������, ������� ��������� �����, ���������� � ����� �����, � ������, �������� � �������� ���������, �� �����.
	if (nMaxStage == MAXSTAGE)
	{ // ����� ������������ ���������� ������ � ������� ���� ����� 20
		MessageBox(hWndMain, TEXT("�������� ������ ������."), TEXT("�����������"), MB_OK);
		CustomFlag = FALSE;
		return FALSE;
	}
	FILE* f;
	char Name[64];
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt �������� ��� ���������������� �����, ����������� �� �����.
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64);
			if (feof(f)) break;
			if (strcmp(FileName, Name) == 0) // ����� ���� ����� � ������, ��������� � �������� ���������, ����������
			{
				MessageBox(hMDlg, TEXT("����� �� ����� ��� ���������."), TEXT("�����������"), MB_OK);
				CustomFlag = FALSE;
				fclose(f);
				return FALSE;
			}
		}
		fclose(f);
	}
	fopen_s(&f, "AddList.txt", "a");
	if (f != NULL)
	{ // ���� ���� ����� � ������, �������� � �������� ���������, �� ����������, AddList.txt ������������.
		fprintf(f, "%s\n", FileName);
		fclose(f);
		memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage])); // arStage � ���������� ������, � ������� �������� ��� �����.
		nMaxStage++;                                                          // ��������� ����������� ���������������� �����.
	}                                                                         // ����������� ����� ���������� ������ � ������� ����.
	return TRUE;
}

void LoadAddStage()
{ // �������, ������� ���������� �����, ������� ���� ��������� ��� ������ ������� ����, � ��������� �� � �������, � ������� �������� ��� �����.
	FILE* f;
	FILE* fr;
	char Name[64];
	char rmName[17][64]; // ������������ ���������� ����, ������� ����� ��������, � 17.
	int i = 0;
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r");
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64); // ������ ������ ����� ����� �����
			if (feof(f)) break;
			fopen_s(&fr, Name, "r"); // �������� ����������� ���� ����� ��� ��������� ������ ��� ������
			if (fr == NULL)
			{ // ���� ����������� ���� ����� �� ����������
				strcpy_s(rmName[i], 64, Name); // � AddList.txt ��������� ��� ��������������� ����� ����� � ��������� �������, ����� ������� ���.
				i++;
				continue;
			}
			fclose(fr);
			MapLoader(Name); // ���� ����������� ���� ����� ����������, ����� ����������� � ����������� � ������� Custom Stage.
			memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage]));
			nMaxStage++; // ��������� ����������� ����� � ������� arStage, � ������� �������� ��� �����. ����������� ����� ���������� ������ � ������� ����.���Ѵ�
		}
		fclose(f);
	}
	for (int j = 0; j < i; j++)
	{
		Remove("AddList.txt", rmName[j]); // ������� ����� �������������� ������ ����, ��������� ������, � ������� �������� ����� �������������� ������ ����.
	}
}

void Remove(char* Name, char* rm)
{ // ������� �������� ����� ��������������� ����� ����� �� AddList.txt
	FILE* fr;
	FILE* fw;
	char tempstr[64];
	fopen_s(&fr, Name, "r");
	fopen_s(&fw, "temp.txt", "w");
	if (fr != NULL && fw != NULL)
	{
		while (TRUE)
		{
			fscanf_s(fr, "%s", tempstr, 64); // ��������� ���� ��� �� AddList.txt
			if (feof(fr)) break;
			if (strcmp(rm, tempstr) != 0) // ���� ��� �� ��������� � ������ ��������������� ����� �����
			{
				fprintf(fw, "%s\n", tempstr); // �������� � temp.txt ��� �� ������� ��� ��������������� ����� �����.
			}
		}
		fclose(fr);
		fclose(fw);
	}
	remove(Name); // ������� ���� AddList.txt.
	rename("temp.txt", Name); // ������������ temp.txt � AddList.txt.
}