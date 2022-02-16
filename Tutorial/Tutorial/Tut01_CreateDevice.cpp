//-----------------------------------------------------------------------------
// 파일:	Tut01_CreateDevice.cpp
//
// 설명:	D3D 디바이스를 생성하고, 이를 사용해서 화면을 지우는 방법에 대해서 공부하게 된다.
//
//	Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include <Windows.h>
// Direct3D9를 사용하기 위한 헤더
#include <d3d9.h>

#pragma warning(disable: 28251)	// WinMain 주석 오류 경고

//-----------------------------------------------------------------------------
// 전역 변수
//-----------------------------------------------------------------------------
LPDIRECT3D9         g_pD3D = NULL;				// D3D 디바이스를 생성할 D3D 객체 변수
LPDIRECT3DDEVICE9   g_pd3dDevice = NULL;		// 렌더링에 사용될 D3D 디바이스




//-----------------------------------------------------------------------------
// Direct3D 초기화
//-----------------------------------------------------------------------------
HRESULT InitD3D(HWND hWnd)
{
	// 디바이스를 생성하기 위한 D3D 객체 생성
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;

	D3DPRESENT_PARAMETERS d3dpp;				// 디바이스 생성을 위한 구조체
	ZeroMemory(&d3dpp, sizeof(d3dpp));			// 반드시 ZeroMemory() 함수로 미리 구조체를 깨끗이 지워야 함
	d3dpp.Windowed = TRUE;						// 창 모드로 생성
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	// 가장 효율적인 SWAP 효과
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;	// 현재 배경화면 모드에 맞춰서 후면 버퍼를 생성

	// 디바이스를 다음과 같은 설정으로 생성한다.
	// 1. 디폴트 비디오카드를 사용한다(대부분은 비디오카드가 한 개다).
	// 2. HAL 디바이스를 생성한다(HW 가속 장치를 사용하겠다는 의미).
	// 3. 정점 처리는 모든 카드에서 지원하는 SW 처리로 생성한다(HW로 생성할 경우 더욱 높은 성능을 낸다).
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}

	// 디바이스 상태 정보를 처리할 경우 여기에서 한다.

	return S_OK;
}




//-----------------------------------------------------------------------------
// 초기화된 객체들을 소거한다
//-----------------------------------------------------------------------------
VOID Cleanup()
{
	if (g_pd3dDevice != NULL)
		g_pd3dDevice->Release();

	if (g_pD3D != NULL)
		g_pD3D->Release();
}




//-----------------------------------------------------------------------------
// 화면 그리기
//-----------------------------------------------------------------------------
VOID Render()
{
	if (NULL == g_pd3dDevice)
		return;

	// 후면 버퍼를 파란색(0, 0, 255)으로 지운다.
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

	// 렌더링 시작
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// 실제 렌더링 명령들이 나열될 곳

		// 렌더링 종료
		g_pd3dDevice->EndScene();
	}

	// 후면 버퍼를 전면 버퍼와 전환
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}




//-----------------------------------------------------------------------------
// 윈도우 프로시저
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
		Render();
		ValidateRect(hWnd, NULL);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}




//-----------------------------------------------------------------------------
// 이 프로그램의 시작점
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// 윈도우 클래스 등록
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		"D3D Tutorial", NULL
	};
	RegisterClassEx(&wc);

	// 윈도우 생성
	HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 01: CreateDevice",
		WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
		NULL, NULL, wc.hInstance, NULL);

	// Direct3D 초기화
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		// 윈도우 출력
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);

		// 메세지 루프 진입
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// 등록된 클래스 소거
	UnregisterClass("D3D Tutorial", wc.hInstance);
	return 0;
}