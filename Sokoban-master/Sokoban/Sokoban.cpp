#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <direct.h>
#include "resource.h"
using namespace std;

#pragma comment(lib,"winmm.lib")

#define MAXSTAGE 20 // Количество ступеней
#define BW 32 // Ширина растрового изображения, которое будет использоваться для вывода
#define BH 32 // Высота растрового изображения, которое будет использоваться для вывода
#define MAXUNDO 1000 // Максимальное количество, которое может хранить информацию об отмене перемещения
#define LOAD_LEN 21
#define MAXSTRING 512 // Максимальная длина, которую можно сохранить в массиве символов
#define NOTICE TEXT("Пожалуйста, введите имя файла (только на английском языке).txt.")

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
				MessageBox(HWND_DESKTOP, TEXT("Нет сохраненных пользовательских карт. Инициализировать до этапа 1."), TEXT("Уведомление"), MB_OK);
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
					wsprintf(Message, TEXT("%d Я разблокировал сцену. Перейти к следующему этапу."), nStage + 1);
				}
				MessageBox(hWnd, (LPWSTR)Message, TEXT("Уведомление"), MB_OK);
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
		case ID_LOADCUSTOM: // ID загрузки пользовательской карты из меню
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_MAKEMAP), hWnd, MapDlgProc) == IDOK)
			{ // IDD_MAKEMAP — это идентификатор пользовательского диалогового окна импорта карт.
				memset(NameToChar, 0, sizeof(char) * 64);
				WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), NameToChar, 64, 0, 0); // Преобразование типа данных TCHAR* в тип данных char*
				MapLoader(NameToChar); // Загрузите файл карты, соответствующий названию карты.
				CustomFlag = TRUE; // Флаг, указывающий, использовалась ли загрузка пользовательской карты
				if (AddStage(NameToChar)) // Добавить загруженную карту в качестве следующего этапа после последнего этапа текущей игры.
				{
					InitStage(); // Сброс уровня с загруженной картой
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
			MessageBox(hWnd, TEXT("Создатель: Резников Данил\nE-mail: danylo.reznikov@nure.ua"), TEXT("Информация"), MB_OK);
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
{ // Процедура, которая обрабатывает сообщения из диалоговых окон справки. немодальный
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
		return TRUE; // Создайте таблицу строк и загрузите строку в каждый идентификатор.
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
{ // Процедура обработки сообщений из диалогового окна «Загрузить пользовательскую карту». модальный
	FILE* f;
	FILE* IsF;
	TCHAR FileName[64];
	char ExistFile[64];
	int i = 0;
	switch (iMessage)
	{
	case WM_INITDIALOG:
		hMapDlg = hDlg;
		fopen_s(&f, "MapList.txt", "r"); // MapList.txt — это файл, содержащий список имен добавленных карт.
		if (f != NULL)                   // Название каждой карты записывается в MapList.txt
		{
			while (TRUE)
			{
				memset(ExistFile, 0, sizeof(char) * 64);
				fscanf_s(f, "%ls", FileName, 64);
				if (feof(f)) break;
				WideCharToMultiByte(CP_ACP, 0, FileName, lstrlen(FileName), ExistFile, 64, 0, 0); // Преобразование типа данных TCHAR* в тип данных char*
				fopen_s(&IsF, ExistFile, "r"); // Прочитайте одно из имен карт и откройте файл только для чтения, чтобы проверить, существует ли файл карты. 
				if (IsF != NULL)
				{
					SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_ADDSTRING, i, (LPARAM)FileName); // Если файл карты существует, добавьте его в список
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
		case IDC_CMAPLIST: // ID списка
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				i = SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETCURSEL, 0, 0); // Сохраните номер списка в поле списка, в котором находится выбранный список.
				SendMessage(GetDlgItem(hDlg, IDC_CMAPLIST), LB_GETTEXT, i, (LPARAM)CMapName); // Сохраните строку в i-м списке в CMapName
				SetDlgItemText(hDlg, IDC_LOADMAP, CMapName); // IDC_LOADMAP — это идентификатор редактирования. Вывести строку выбранного списка для редактирования
				return TRUE;
			}
			return FALSE;
		case IDOK:
			GetDlgItemText(hDlg, IDC_LOADMAP, CMapName, 64); // Прочитать строку в редактировании
			if (lstrcmp(CMapName, NOTICE) == 0)
			{ // Если прочитанная строка совпадает со строкой, установленной по умолчанию, всплывающее окно сообщения уведомляет
				MessageBox(hMapDlg, TEXT("Пожалуйста, введите правильное название карты."), TEXT("Уведомление"), MB_OK);
				return TRUE;
			}
			memset(ExistFile, 0, sizeof(char) * 64);
			WideCharToMultiByte(CP_ACP, 0, CMapName, lstrlen(CMapName), ExistFile, 64, 0, 0);
			fopen_s(&f, ExistFile, "r");
			if (f == NULL)
			{ // Откройте имя карты только для чтения, чтобы проверить ее существование. Если нет, всплывающее окно сообщения, чтобы уведомить
				MessageBox(hMapDlg, TEXT("Нет карты."), TEXT("Уведомление"), MB_OK);
				return TRUE;
			}
			if (!IsDuplicated(CMapName)) // Проверить наличие дубликатов одного и того же имени карты
			{
				MakeMapList(CMapName); // Если повторяющихся имен нет, то записывается в MapList.txt
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
	wsprintf(Message, TEXT("Q:Конец, R:Перезапуск"));
	TextOut(hdc, 700, 30, Message, lstrlen(Message));
	wsprintf(Message, TEXT("N:Следующий, P:Предыдущий"));
	TextOut(hdc, 700, 50, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Z:Отмена, Y:Переделывать"));
	TextOut(hdc, 700, 70, Message, lstrlen(Message));
	wsprintf(Message, TEXT("S:Звук вкл/выкл"));
	TextOut(hdc, 700, 90, Message, lstrlen(Message));
	if (nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("Этап : %d"), nStage + 1);
	}
	TextOut(hdc, 700, 120, Message, lstrlen(Message));
	wsprintf(Message, TEXT("Количество ходов : %d"), nMove);
	TextOut(hdc, 700, 140, Message, lstrlen(Message));
	if (RecordsSize[nStage] != 0 && nStage < nMaxStage)
	{
		wsprintf(Message, TEXT("Рекордный ранг"));
		TextOut(hdc, 700, 180, Message, lstrlen(Message));
		for (int i = 0; i < RecordsSize[nStage]; i++)
		{
			wsprintf(Message, TEXT("Выше: %d"), i + 1, Records[nStage][i]);
			TextOut(hdc, 700, 180 + 20 * (i + 1), Message, lstrlen(Message));
		}
	}
}

BOOL TestEnd()
{ // Функция, которая проверяет, расположены ли ящики во всех целевых местах.
	int x, y;
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (TempStage[y][x] == '.' && ns[y][x] != '!') //  Когда ящики расположены не во всех целевых позициях 
			{
				return FALSE;
			}
		}
	}
	/*if (nStage < nMaxStage)
	{
		SortRecords(Records[nStage]);
	}*/
	SortRecords(Records[nStage]); // Функция сортировки истории текущего этапа
	return TRUE;
}

void Move(int dir)
{
	int dx = 0, dy = 0; // Переменные, определяющие расстояние и направление движения
	BOOL bWithPack = FALSE; // Флаг для создания звука, различающего, когда персонаж движется в одиночку, и когда персонаж перемещает груз.
	BOOL Success = FALSE; // Флаг, чтобы издавать звук, когда коробка помещается в целевую позицию 
	int i;
	switch (dir)
	{
	case VK_LEFT:
		ManBit = 4; // ManBit — растровое изображение персонажа. Укажите разные растровые изображения для верхнего, нижнего, левого и правого
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
	if (ns[ny + dy][nx + dx] != '#') // ns — это массив, в котором хранится статус текущего этапа, обновляемый при каждом ходе.
	{ // Когда следующий квадрат не стена
		if (ns[ny + dy][nx + dx] == 'O' || ns[ny + dy][nx + dx] == '!')
		{ // Когда следующее пространство является грузом, расположенным в целевой позиции
			if (ns[ny + dy * 2][nx + dx * 2] == ' ' || ns[ny + dy * 2][nx + dx * 2] == '.')
			{ // Далее Когда следующий квадрат является пустым местом или целевой позицией
				ErasePack(nx + dx, ny + dy); // Функция, которая сравнивает текущую стадию с сохраненным массивом и сохраняет символ . или пробел в координатах, заданных в качестве аргумента.
				bWithPack = TRUE;
				if (ns[ny + dy * 2][nx + dx * 2] == '.')
				{ // Когда следующий квадрат является целевой позицией
					Success = TRUE;
					ns[ny + dy * 2][nx + dx * 2] = '!'; // Сохраняет текст, представляющий нагрузку, расположенную в целевом положении
				}
				else if (ns[ny + dy * 2][nx + dx * 2] == ' ')
				{ // Когда следующее место пусто
					ns[ny + dy * 2][nx + dx * 2] = 'O'; // сохранить символ, представляющий нагрузку
				}
			}
			else
			{
				return;
			}
		}
		nx += dx;
		ny += dy;
		nMove++; // увеличение количества ходов
		MOVEINFO[UndoIdx].dx = dx; // 
		MOVEINFO[UndoIdx].dy = dy; //
		MOVEINFO[UndoIdx].bWithPack = bWithPack; // Храните информацию в массиве структур, в которых хранится информация об отмене/повторении перемещения.
		UndoIdx++;
		MOVEINFO[UndoIdx].dx = -2; // Число -2, отличающее место в массиве, где хранится информация, от места в массиве, которое еще не сохранено.
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
	{ // При использовании пользовательской загрузки карты
		/*memcpy(TempStage, nCustomStage, sizeof(TempStage));
		AlreadyDoneCheck();
		memcpy(ns, nCustomStage, sizeof(ns));*/
		nStage = nMaxStage - 1; // Загруженная карта становится завершающим этапом
	}
	memcpy(TempStage, arStage[nStage], sizeof(TempStage)); // Массив arStage — это трехмерный массив, в котором хранится вся карта игры.
	AlreadyDoneCheck(); // Функция, которая преобразует символ ! в символ . и сохраняет его в массиве текущего этапа для сравнения.
	memcpy(ns, arStage[nStage], sizeof(ns));
	for (y = 0; y < 18; y++)
	{
		for (x = 0; x < 20; x++)
		{
			if (ns[y][x] == '@')
			{ // Найдите координаты персонажа и сохраните их в текущей переменной координат
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
	{ // Если бы координаты (x, y) были исходной целевой позицией
		ns[y][x] = '.';
	}
	else
	{ // Если координаты (x, y) изначально были пустым пространством
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
			fprintf(f, "%s", ns[i]); // Сохраните текущий массив сцены
		}
		fprintf(f, "%d", nStage); // сохранить текущий этап
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
	} // Прочитать карту, хранящуюся в массиве ns
	/*if (CustomFlag)
	{
		MapLoader(NameToChar);
	}*/
	LoadAddStage(); // Добавленные этапы загружаются
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt — это файл, содержащий имена добавленных в данный момент карт.
	char tempstr[64];
	memset(tempstr, 0, sizeof(char) * 64);
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", tempstr, 64);
			fopen_s(&fr, tempstr, "r"); // Прочитайте одно имя карты и откройте файл только для чтения
			if (fr == NULL || nStage >= nMaxStage)
			{
				if (fr == NULL)
				{ // Если ничего не написано в AddList.txt
					fclose(f);
					remove("AddList.txt"); // Удалите файл AddList.txt.
					MessageBox(HWND_DESKTOP, TEXT("Произошла ошибка при загрузке добавленной карты. Инициализировать до этапа 1."), TEXT("Уведомление"), MB_OK);
				}
				if (nStage >= nMaxStage)
				{ // Если текущий сохраненный номер этапа больше, чем количество карт, которые есть в игре на данный момент
					MessageBox(HWND_DESKTOP, TEXT("Произошла ошибка. Переупорядочить этапы."), TEXT("Уведомление"), MB_OK);
				}
				errflag = TRUE; // Флаг, указывающий, что произошла ошибка. Инициализировать оценки (записи) добавленной карты
				CustomFlag = FALSE;
				nStage = 0; // Инициализировать до этапа 1.
				break;
			}
			if (fr != NULL) fclose(fr);
			if (feof(f)) break;
		}
		if (f != NULL) fclose(f);
	}
}

void MakeMapList(TCHAR* MapName)
{ // Функция, которая наследует добавленное имя файла карты
	FILE* f;
	fopen_s(&f, "MapList.txt", "a");
	if (f != NULL)
	{
		fprintf(f, "%ls\n", MapName);
		fclose(f);
	}
}

BOOL IsDuplicated(TCHAR* FileName)
{ // Функция, которая проверяет, не дублируется ли имя файла карты, заданное в качестве аргумента.
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
} // Возвращает TRUE, если дублируется, возвращает FALSE, если не дублируется.

BOOL MapLoader(char* FileName)
{ // Функция, которая читает карту, записанную в файле карты с именем файла карты, указанным в качестве аргумента.
	FILE* f;
	int i = 0;
	fopen_s(&f, FileName, "r");
	if (f == NULL) return FALSE;
	else
	{
		while (TRUE)
		{
			fread_s(nCustomStage[i], sizeof(nCustomStage[i]), sizeof(char), 21, f); // Массив Custom Stage — это массив, который считывает и хранит пользовательские карты.
			nCustomStage[i][20] = 0;
			if (feof(f)) break;
			i++;
		}
		fclose(f);
		return TRUE;
	}
}

void SortRecords(int record[])
{ // Функция сортировки записей. Пузырьковую сортировку проще всего использовать, поскольку она сортирует небольшое количество записей.
	BOOL flag = FALSE; // Флаг, указывающий, заполнен ли массив, хранящий записи
	for (int i = 0; i < 5; i++)
	{
		if (record[i] == 0)
		{
			record[i] = nMove;
			RecordsSize[nStage]++; // RecordsSize — это двумерный массив, в котором хранятся записи для каждого этапа. Массив записей каждого этапа имеет максимальную длину 5
			flag = TRUE;
			break;
		}
	}
	if (flag) // Если в массиве остались пустые места, сортировка пузырьком по количеству сохраненных записей
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
	{ // Если в массиве не осталось пустого места, сравните самую низкую запись и, если она лучше, сохраните текущую запись в самой нижней позиции записи и выполните пузырьковую сортировку.
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
{ // Функция, записывающая записи для каждого этапа в бинарный файл
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
{ // Функция, которая читает записи, записанные в файл, и сохраняет их в массиве записей.
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
{ // Функция, которая преобразует символы ! в символы . и сохраняет их.
	for (int i = 0; i < 18; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			TempStage[i][j] = (TempStage[i][j] == '!') ? '.' : TempStage[i][j];
		}
	}
}

BOOL AddStage(char* FileName)
{ // Функция, которая добавляет карту, хранящуюся в файле карты, с именем, заданным в качестве аргумента, на сцену.
	if (nMaxStage == MAXSTAGE)
	{ // Когда максимальное количество этапов в текущей игре равно 20
		MessageBox(hWndMain, TEXT("Добавить больше нельзя."), TEXT("Уведомление"), MB_OK);
		CustomFlag = FALSE;
		return FALSE;
	}
	FILE* f;
	char Name[64];
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r"); // AddList.txt содержит имя пользовательской карты, добавленной на сцену.
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64);
			if (feof(f)) break;
			if (strcmp(FileName, Name) == 0) // Когда файл карты с именем, указанным в качестве аргумента, существует
			{
				MessageBox(hMDlg, TEXT("Такая же карта уже добавлена."), TEXT("Уведомление"), MB_OK);
				CustomFlag = FALSE;
				fclose(f);
				return FALSE;
			}
		}
		fclose(f);
	}
	fopen_s(&f, "AddList.txt", "a");
	if (f != NULL)
	{ // Если файл карты с именем, заданным в качестве аргумента, не существует, AddList.txt продолжается.
		fprintf(f, "%s\n", FileName);
		fclose(f);
		memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage])); // arStage — трехмерный массив, в котором хранится вся сцена.
		nMaxStage++;                                                          // Сохраняет загруженную пользовательскую карту.
	}                                                                         // Увеличивает общее количество этапов в текущей игре.
	return TRUE;
}

void LoadAddStage()
{ // Функция, которая запоминает этапы, которые были добавлены при первом запуске игры, и сохраняет их в массиве, в котором хранятся все этапы.
	FILE* f;
	FILE* fr;
	char Name[64];
	char rmName[17][64]; // Максимальное количество карт, которые можно добавить, — 17.
	int i = 0;
	memset(Name, 0, sizeof(char) * 64);
	fopen_s(&f, "AddList.txt", "r");
	if (f != NULL)
	{
		while (TRUE)
		{
			fscanf_s(f, "%s", Name, 64); // Чтение одного имени файла карты
			if (feof(f)) break;
			fopen_s(&fr, Name, "r"); // Откройте прочитанный файл карты как доступный только для чтения
			if (fr == NULL)
			{ // Если прочитанный файл карты не существует
				strcpy_s(rmName[i], 64, Name); // В AddList.txt сохраните имя несуществующего файла карты в двумерном массиве, чтобы удалить его.
				i++;
				continue;
			}
			fclose(fr);
			MapLoader(Name); // Если прочитанный файл карты существует, карта загружается и сохраняется в массиве Custom Stage.
			memcpy(arStage[nMaxStage], nCustomStage, sizeof(arStage[nMaxStage]));
			nMaxStage++; // Сохраните загруженную карту в массиве arStage, в котором хранится вся сцена. Увеличивает общее количество этапов в текущей игре.°ЎЗСґЩ
		}
		fclose(f);
	}
	for (int j = 0; j < i; j++)
	{
		Remove("AddList.txt", rmName[j]); // Удаляет имена несуществующих файлов карт, используя массив, в котором хранятся имена несуществующих файлов карт.
	}
}

void Remove(char* Name, char* rm)
{ // Функция удаления имени несуществующего файла карты из AddList.txt
	FILE* fr;
	FILE* fw;
	char tempstr[64];
	fopen_s(&fr, Name, "r");
	fopen_s(&fw, "temp.txt", "w");
	if (fr != NULL && fw != NULL)
	{
		while (TRUE)
		{
			fscanf_s(fr, "%s", tempstr, 64); // Прочитать одно имя из AddList.txt
			if (feof(fr)) break;
			if (strcmp(rm, tempstr) != 0) // Если оно не совпадает с именем несуществующего файла карты
			{
				fprintf(fw, "%s\n", tempstr); // написать в temp.txt Это не запишет имя несуществующего файла карты.
			}
		}
		fclose(fr);
		fclose(fw);
	}
	remove(Name); // Удалите файл AddList.txt.
	rename("temp.txt", Name); // Переименуйте temp.txt в AddList.txt.
}