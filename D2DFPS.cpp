/********************************************************************
created:	2014/06/14
author:		Baesky(http://www.cnblogs.com/Baesky/)

purpose:	Direct2D FPS Demo
*********************************************************************/
#include "D2DFPS.h"

#define SAFE_RELEASE(p) { if(p) {(p)->Release(); (p)=NULL;}}
#define KEY_DOWN(VK_CODE) ( (GetAsyncKeyState(VK_CODE) & 0x8000) ? 1 : 0 )

#define MAX_double (3.4e+38f)
#define SCREEN_RESOLUTION 320
#define CAMERA_FOV (PI * 0.4)
static const double PI = 3.1415926535897932;
#define PI2 (PI*2.0)
#define CAM_ROT_SPEED 0.4
#define CAM_MOVE_SPEED 0.5

static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 480;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			D2DFPS app;
			if (SUCCEEDED(app.Initialize()))
			{
				app.RunMsgLoop();
			}

		}
		CoUninitialize();
	}

	return 0;
}

D2DFPS::D2DFPS():
	m_hWnd(NULL),
	m_pD2DFactory(NULL),
	m_pWICFactory(NULL),
	m_pBitmap(NULL),
	m_pRenderTarget(NULL),
	m_pBlackBrush(NULL),
	m_pWallBrush(NULL),
	m_pBitmapBrush(NULL),
	m_pDwriteFactory(NULL),
	m_pTextFormat(NULL)
{

}

D2DFPS::~D2DFPS()
{
	SAFE_RELEASE(m_pD2DFactory);
	SAFE_RELEASE(m_pWICFactory);
	SAFE_RELEASE(m_pBitmap);
	SAFE_RELEASE(m_pRenderTarget);
	SAFE_RELEASE(m_pWallBrush);
	SAFE_RELEASE(m_pBlackBrush);
	SAFE_RELEASE(m_pBitmapBrush);
	SAFE_RELEASE(m_pDwriteFactory);
	SAFE_RELEASE(m_pTextFormat);

	delete[] m_Map->m_Grid;
}

HRESULT D2DFPS::Initialize()
{
	HRESULT hr;
	srand((int)time(0));
	InitTiming();
	m_Player = new Player(15.3f, -1.2f, PI * 0.3f);
	m_Map = new Map(32);
	m_Map->Randomize();
	m_Player->SetMap(m_Map);
	m_Camera = new Camera();
	hr = CreateDeviceIndependentResources();
	if (SUCCEEDED(hr))
	{
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = D2DFPS::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpszClassName = TEXT("D2DFPSDemo");

		RegisterClassEx(&wcex);

		float dpiX, dpiY;
		m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

		m_hWnd = CreateWindow(
			TEXT("D2DFPSDemo"),
			TEXT("Direct2D FPS Demo - http://www.cnblogs.com/Baesky"),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
			static_cast<UINT>(ceil(480.f * dpiX / 96.f)),
			NULL,
			NULL,
			HINST_THISCOMPONENT,
			this);

		hr = m_hWnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			ShowWindow(m_hWnd, SW_SHOWNORMAL);
			UpdateWindow(m_hWnd);
		}
	}

	return hr;
}

HRESULT D2DFPS::CreateDeviceIndependentResources()
{
	HRESULT hr;

	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pWICFactory));

	if (SUCCEEDED(hr))
	{
		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&m_pD2DFactory);
	}

	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_pDwriteFactory));
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pDwriteFactory->CreateTextFormat(
			L"Verdana",
			NULL,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			10.5f,
			L"en-us",
			&m_pTextFormat);
	}

	return hr;
}

HRESULT D2DFPS::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!m_pRenderTarget)
	{
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top);

		hr = m_pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hWnd, size),
			&m_pRenderTarget);

		if (SUCCEEDED(hr))
		{
			hr = m_pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White, 1.0f),
				&m_pBlackBrush
				);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF(0x9ACD32, 1.0f)),
				&m_pWallBrush);
		}

		if (SUCCEEDED(hr))
		{
			hr = LoadBitmapFromFile(
				m_pRenderTarget,
				m_pWICFactory,
				L"..\\test.bmp",
				633,
				349,
				&m_pBitmap
				);
		}
		if (SUCCEEDED(hr))
		{
			hr = m_pRenderTarget->CreateBitmapBrush(
				m_pBitmap,
				&m_pBitmapBrush);
		}

	}
	return hr;
}

void D2DFPS::DiscardDeviceResources()
{
	SAFE_RELEASE(m_pRenderTarget);
	SAFE_RELEASE(m_pBlackBrush);
	SAFE_RELEASE(m_pWallBrush);
	SAFE_RELEASE(m_pBitmapBrush);
}

void D2DFPS::RunMsgLoop()
{
	MSG msg;

	while (true)
	{
		GCurrentTime = GetCurrentTime();
		GDeltaTime = GCurrentTime - GLastTime;
		GAccumulateTime += GDeltaTime;
		if (GAccumulateTime >= 1000)
		{
			GFps = GCounter;
			GAccumulateTime = 0;
			GCounter = 0;
		}
		else
		{
			GCounter++;
		}
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update(GDeltaTime);
			Render();
			if (KEY_DOWN(VK_ESCAPE))
			{
				PostQuitMessage(0);
				break;
			}
		}
		GLastTime = GCurrentTime;
	}
	
}

HRESULT D2DFPS::OnRender()
{
	HRESULT hr = CreateDeviceResources();

	if (SUCCEEDED(hr))
	{
		wchar_t szSolidBrushText[10];
		_itow_s(GFps, szSolidBrushText, 10);

		m_pRenderTarget->BeginDraw();

		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		D2D1_RECT_F rcTextRect = D2D1::RectF(5, 5, 35, 35);

		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

		DrawColumns();

		m_pRenderTarget->DrawText(szSolidBrushText,
			wcslen(szSolidBrushText),
			m_pTextFormat,
			&rcTextRect,
			m_pBlackBrush);

		hr = m_pRenderTarget->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET)
		{
			hr = S_OK;
			DiscardDeviceResources();
		}
	}

	return hr;
}

void D2DFPS::OnResize(UINT width, UINT height)
{
	if (m_pRenderTarget)
	{
		m_pRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}

LRESULT CALLBACK D2DFPS::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		D2DFPS* pD2dFps = (D2DFPS*)pcs->lpCreateParams;

		::SetWindowLongPtrW(hWnd, GWLP_USERDATA, PtrToUlong(pD2dFps));

		result = 1;
	}
	else
	{
		D2DFPS* pD2dFps = reinterpret_cast<D2DFPS*>(static_cast<LONG_PTR>(
			::GetWindowLongPtrW(hWnd, GWLP_USERDATA)));

		bool wasHandled = false;

		if (pD2dFps)
		{
			switch (message)
			{
			case WM_SIZE:
				{
					UINT width = LOWORD(lParam);
					UINT height = HIWORD(lParam);
					pD2dFps->OnResize(width, height);
				}
				result = 0;
				wasHandled = true;
				break;

			case WM_DISPLAYCHANGE:
				{
					InvalidateRect(hWnd, NULL, FALSE);
				}
				result = 0;
				wasHandled = true;
				break;

			case WM_PAINT:
				{
					pD2dFps->OnRender();
					ValidateRect(hWnd, NULL);
				}
				result = 0;
				wasHandled = true;
				break;

			case WM_DESTROY:
				{
				PostQuitMessage(0);
				}
				result = 1;
				wasHandled = true;
				break;
			}
		}

		if (!wasHandled)
		{
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return result;
}

HRESULT D2DFPS::LoadResourceBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap **ppBitmap)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;

	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void*  pImageFile = NULL;
	DWORD imageFileSize = 0;

	imageResHandle = FindResourceW(HINST_THISCOMPONENT, resourceName, resourceType);

	hr = imageResHandle ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		imageResDataHandle = LoadResource(HINST_THISCOMPONENT, imageResHandle);
		hr = imageResDataHandle ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		pImageFile = LockResource(imageResDataHandle);
		hr = pImageFile ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		imageFileSize = SizeofResource(HINST_THISCOMPONENT, imageResHandle);
		hr = imageFileSize ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateStream(&pStream);
	}
	if (SUCCEEDED(hr))
	{
		hr = pStream->InitializeFromMemory(
			reinterpret_cast<BYTE*>(pImageFile),
			imageFileSize
			);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateDecoderFromStream(
			pStream, NULL, WICDecodeMetadataCacheOnLoad,
			&pDecoder);
	}
	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pSource);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}
	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut);
	}
	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter, NULL, ppBitmap);
	}

	SAFE_RELEASE(pDecoder);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pConverter);

	return hr;
}

HRESULT D2DFPS::LoadBitmapFromFile(ID2D1RenderTarget *pRenderTarget, IWICImagingFactory *pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap **ppBitmap)
{
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pSource = NULL;
	IWICStream *pStream = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmapScaler *pScaler = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		uri,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
		);

	if (SUCCEEDED(hr))
	{
		// Create the initial frame.
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr))
	{

		// Convert the image format to 32bppPBGRA
		// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
		hr = pIWICFactory->CreateFormatConverter(&pConverter);

	}

	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut
			);
	}

	if (SUCCEEDED(hr))
	{

		// Create a Direct2D bitmap from the WIC bitmap.
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			ppBitmap
			);
	}

	SAFE_RELEASE(pDecoder);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pConverter);
	SAFE_RELEASE(pScaler);

	return hr;
}

void D2DFPS::Update(double DeltaTime)
{
	double ms = DeltaTime / 1000.0f;
	m_Map->Update(ms);
	m_Player->Update(ms);

}

void D2DFPS::DrawColumns()
{
	for (int column = 0; column < SCREEN_RESOLUTION; ++column)
	{
		double angle = m_Camera->fov * ((float)column / SCREEN_RESOLUTION - 0.5);
		Ray ray = m_Map->Cast(m_Map, m_Player->GetLocation(), angle + m_Player->Direction, m_Camera->range);
		DrawColumn(column, ray, angle);
	}
}

void D2DFPS::DrawColumn(double column, Ray ray, double angle)
{
	double left = floor(column*m_Camera->spacing);
	double width = ceil(m_Camera->spacing);
	int hit = -1;
	int StepSize = (int)ray.steps.size();
	while (++hit < StepSize && ray.steps[hit].height <= 0);

	for (int s = ray.steps.size() - 1; s >= 0; s--)
	{
		StepInfo step = ray.steps[s];
		if (s == hit)
		{
			double TextureX = floor(/* texture width * */step.offset);
			Projection wall = Project(step.height, angle, step.distance);
			D2D1_RECT_F rcBrushRect = D2D1::RectF((float)left, (float)wall.top, (float)(left + width), (float)(wall.top + wall.height));
			float alpha = max( (float)((step.distance + step.shading) / m_Camera->lightRange /*- m_Map->m_fLight*/), .0f);
			m_pWallBrush->SetColor(D2D1::ColorF(0x9ACD32, 1.0f - alpha));
			m_pRenderTarget->DrawRectangle(&rcBrushRect, m_pWallBrush, 1, NULL);
		}
	}

}

Projection D2DFPS::Project(double height, double angle, double distance)
{
	double z = distance * cos(angle);
	double wallHeight = height * m_Camera->height / z;
	double bottom = m_Camera->height / 2 * (1 + 1 / z);
	return Projection(bottom - wallHeight, wallHeight);
}

void D2DFPS::Render()
{
	OnRender();
}

void Map::Randomize()
{
	for (UINT i = 0; i < m_Size*m_Size; ++i)
	{
		m_Grid[i] = ((double)rand() / RAND_MAX) < 0.3 ? 1 : 0;
	}
}

Map::Map()
{
}

Map::Map(UINT Size)
{
	m_Size = Size;
	m_fLight = 0;
	m_Grid = new UINT[Size*Size];
}

void Map::Update(double seconds)
{
	if (m_fLight > 0) 
		m_fLight = max(m_fLight - 10 * seconds, 0);
	else if (rand()/RAND_MAX * 5 < seconds )
	{
		m_fLight = 2;
	}
}

Map::~Map()
{
	if (m_Grid)
		delete[] m_Grid;
}

int Map::Get(double x, double y)
{
	x = floor(x);
	y = floor(y);
	if (x < 0 || (x >(m_Size - 1)) || y < 0 || (y >(m_Size - 1)))
		return -1;
	return m_Grid[(int)(y*m_Size + x)];
}

Ray Map::Cast(Map* map, Location Loc, double Angle, double range)
{
	StepInfo SI = StepInfo(Loc.x, Loc.y, 0, 0, 0, 0, 0);
	return Ray(map, SI, sin(Angle), cos(Angle), range);
}

Ray::Ray(Map* map, StepInfo& origin, double Sin, double Cos, double range)
{
	m_map = map;
	sin = Sin;
	cos = Cos;
	
	Cast(origin, range);
}

void Ray::Cast(StepInfo& origin, double range)
{
	StepInfo stepX = Step(sin, cos, origin.x, origin.y, false);
	StepInfo stepY = Step(cos, sin, origin.y, origin.x, true);
	StepInfo nextStep = stepX.length2 < stepY.length2 ? Inspect(stepX, 1, 0, origin.distance, stepX.y)
		: Inspect(stepY, 0, 1, origin.distance, stepY.x);

	steps.push_back(origin);
	if (nextStep.distance < range)
	{
		Cast(nextStep, range);
	}
}

StepInfo Ray::Step(double rise, double run, double x, double y, bool inverted)
{
	if (run == 0) return StepInfo(0, 0, 0, 0, MAX_double, 0, 0);
	double dx = run > 0 ? floor(x + 1) - x : ceil(x - 1) - x;
	double dy = dx * (rise / run);
	return StepInfo(inverted ? y + dy : x + dx, inverted ? x + dx : y + dy, 0, 0, dx*dx + dy*dy, 0, 0);
}

StepInfo Ray::Inspect(StepInfo& step, double shiftX, double shiftY, double distance, double offset)
{
	double dx = cos < 0 ? shiftX : 0;
	double dy = sin < 0 ? shiftY : 0;
	step.height = m_map->Get(step.x - dx, step.y - dy);
	step.distance = distance + sqrt(step.length2);
	if (shiftX == 1)
		step.shading = cos < 0 ? 2 : 0;
	else
		step.shading = sin < 0 ? 2 : 1;
	step.offset = offset - floor(offset);
	return step;
}

StepInfo::StepInfo(double X, double Y, double Height, double Dist, double Lengh2, double Shading, double Offset):
	x(X), y(Y), height(Height), distance(Dist), length2(Lengh2), shading(Shading), offset(Offset)
{

}

void Player::Update(double seconds)
{
	double finalRotSpeed = seconds * CAM_ROT_SPEED;
	double finalMoveSpeed = seconds * CAM_MOVE_SPEED;
	if (KEY_DOWN(VK_LEFT))
	{
		Rotate(-PI * finalRotSpeed);
	}
	if (KEY_DOWN(VK_RIGHT))
	{
		Rotate(PI * finalRotSpeed);
	}
	if (KEY_DOWN(VK_UP))
	{
		Walk(3 * finalMoveSpeed);
	}
	if (KEY_DOWN(VK_DOWN))
	{
		Walk(-3 * finalMoveSpeed);
	}
}

Player::Player(double x, double y, double dir) :PosX(x), PosY(y), Direction(dir), Peace(0)
{

}

Location Player::GetLocation()
{
	return Location(PosX, PosY);
}

void Player::Walk(double dist)
{
	double dx = cos(Direction) * dist;
	double dy = sin(Direction) * dist;

	if (m_map->Get(PosX + dx, PosY) <= 0)
		PosX += dx;
	if (m_map->Get(PosX, PosY + dy) <= 0)
		PosY += dy;
	Peace += dist;
}

void Player::SetMap(Map* map)
{
	m_map = map;
}

void Player::Rotate(double angle)
{
	Direction = fmod((Direction + angle + PI2), PI2);
}

Camera::Camera()
{
	width = SCREEN_WIDTH;
	height = SCREEN_HEIGHT;
	resoultion = SCREEN_RESOLUTION;
	spacing = SCREEN_WIDTH / SCREEN_RESOLUTION;
	fov = CAMERA_FOV;
	range = 14;
	lightRange = 9;
	scale = (width + height) / 1200;
}

Location::Location(double X, double Y) :x(X), y(Y)
{

}

Projection::Projection(double Top, double Height) : top(Top), height(Height)
{

}
