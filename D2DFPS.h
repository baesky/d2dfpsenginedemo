/********************************************************************
	created:	2014/06/14
	author:		Baesky(http://www.cnblogs.com/Baesky/)
	
	purpose:	Direct2D FPS Demo
*********************************************************************/
#ifndef D2DFPS_h__
#define D2DFPS_h__

#ifndef WINVER
#define WINVER 0x0700
#endif // !WINVER

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0700
#endif

#ifndef UNICODE
#define UNITCODE
#endif

#define WIN32_LEAN_AND_MEAN
#include "resource.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "Dwrite.lib")

using namespace std;

static double GSecondsPerCycle = 0.0;
static double GDeltaTime = 0.0;
static double GLastTime = 0.0;
static double GCurrentTime = 0.0;
static int GFps = 0;
static double GAccumulateTime = 0;
static int GCounter = 0;

struct Ray;

struct Location
{
	double x;
	double y;
	Location(double X, double Y);
};

struct Map
{
	UINT m_Size;
	UINT* m_Grid;
	double m_fLight;
	Map();
	Map(UINT Size);
	~Map();
	int Get(double x, double y);
	void Randomize();
	Ray Cast(Map* map, Location Loc, double Angle, double range);
	void Update(double seconds);
};

struct Player
{
	double PosX;
	double PosY;
	double Direction;
	double Peace;
	Map* m_map;

	Player(double x, double y, double dir);
	void Rotate(double angle);
	void Walk(double dist);
	void Update(double seconds);
	Location GetLocation();
	void SetMap(Map* map);
};

struct StepInfo
{
	double x;
	double y;
	double height;
	double distance;
	double length2;
	double shading;
	double offset;

	StepInfo(double X, double Y, double Height, double Dist, double Lengh2, double Shading, double Offset);
};

struct Ray
{
	Map* m_map;
	double sin;
	double cos;
	vector<StepInfo> steps;

	Ray(Map* map, StepInfo& origin, double Sin, double Cos, double range);
	void Cast(StepInfo& origin, double range);
	StepInfo Step(double rise, double run, double x, double y, bool inverted);
	StepInfo Inspect(StepInfo& step, double shiftX, double shiftY, double distance, double offset);

};

struct Camera
{
	int width;
	int height;
	int resoultion;
	double spacing;
	double fov;
	double range;
	double lightRange;
	double scale;

	Camera();
};

struct Projection
{
	double top;
	double height;

	Projection(double Top, double Height);
};

double GetCurrTime()
{
	LARGE_INTEGER Cycles;
	QueryPerformanceCounter(&Cycles);
	return Cycles.QuadPart * GSecondsPerCycle + 16777216.0;
}

double InitTiming()
{
	LARGE_INTEGER Frequency;
	if(QueryPerformanceFrequency(&Frequency))
		GSecondsPerCycle = 1.0 / Frequency.QuadPart;
	return GetCurrentTime();
}

class D2DFPS
{
public:
	D2DFPS();
	~D2DFPS();

	HRESULT Initialize();

	void RunMsgLoop();
	void Update(double DeltaTime);
	void DrawColumns();
	void DrawColumn(double column, Ray ray, double angle);
	Projection Project(double height, double angle, double distance);
	void Render();
private:
	Map* m_Map;
	Player* m_Player;
	Camera* m_Camera;


private:
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();

	HRESULT OnRender();
	HRESULT LoadBitmapFromFile(ID2D1RenderTarget *pRenderTarget, IWICImagingFactory *pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap **ppBitmap);
	HRESULT LoadResourceBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap **ppBitmap);
	void OnResize(UINT width, UINT height);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND m_hWnd;
	ID2D1Factory *m_pD2DFactory;

	IWICImagingFactory *m_pWICFactory;
	ID2D1Bitmap *m_pBitmap;
	ID2D1HwndRenderTarget *m_pRenderTarget;
	ID2D1SolidColorBrush *m_pBlackBrush;
	ID2D1SolidColorBrush *m_pWallBrush;

	ID2D1BitmapBrush* m_pBitmapBrush;

	IDWriteFactory* m_pDwriteFactory;
	IDWriteTextFormat* m_pTextFormat;

};




#endif // D2DFPS_h__
