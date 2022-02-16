//-----------------------------------------------------------------------------
// 파일:	Tut03_Matrices.cpp
//
// 설명:	디바이스와 정점을 생성하는 법을 알았다.
//		3차원 정점을 자유자재로 다루기 위해서는 4x4 크기의 행렬을 사용해야 한다.
//		기본적인 행렬 변환에는 이동(translations), 회전(rotations), 크기(scaling)가 있다.
//		기하 정보는 모델 좌표계를 사용하는데, 이를 우선 3차원 월드 좌표계로 변환해야 한다.
//		이 때 월드 행렬이 사용된다. 다시 월드 좌표계의 기하 정보를 카메라 좌표계로 변환한다.
//		이 때 사용되는 것이 뷰 행렬이다. 다시 이 기하 정보를 2차원상의 평면(viewport)에 투영해야
//		윈도우에 그려질 수 있다. 즉, 월드->뷰->프로젝션의 행렬 변환을 거친 뒤에 비로소
//		그려질 수 있는 것이다(물론, 이후 클리핑 등의 처리가 추가적으로 이루어진다).
//
//		OpenGL에서는 행렬 연산 함수를 직접 작성해야 하겠지만, D3D에는 D3DX라는
//		유틸리티 함수들이 다수 존재한다. 여기서는 D3DX 계열 함수를 사용할 것이다.
//
//	Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <mmsystem.h>			// timeGetTime() 함수를 사용하기 위해서 포함하는 헤더
#include <d3dx9.h>

#pragma warning(disable: 28251)	// WinMain 주석 오류 경고

//-----------------------------------------------------------------------------
// 전역 변수
//-----------------------------------------------------------------------------
LPDIRECT3D9				g_pD3D = NULL;			// D3D 디바이스를 생성할 D3D 객체 변수
LPDIRECT3DDEVICE9		g_pd3dDevice = NULL;	// 렌더링에 사용될 D3D 디바이스
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;			// 정점을 보관할 정점 버퍼

// 사용자 정점을 정의할 구조체
struct CUSTOMVERTEX
{
	FLOAT x, y, z;	// 정점의 3차원 좌표
	DWORD color;	// 정점의 색깔
};

// 사용자 정점 구조체에 관한 정보를 나타내는 FVF 값
// 구조체는 X, Y, Z 값과 Diffuse 색깔 값으로 이루어져 있음을 알 수 있다.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

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

	// 컬링 기능을 끈다. 삼각형의 앞면, 뒷면을 모두 렌더링한다
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// 정점에 색깔값이 있으므로 광원 기능을 끈다.
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	return S_OK;
}

//-----------------------------------------------------------------------------
// 기하 정보 초기화
//-----------------------------------------------------------------------------
// 정점 버퍼를 생성하고 정점값을 채워넣는다.
// 정점 버퍼란 기본적으로 정점 정보를 갖고 있는 메모리 블록이다.
// 정점 버퍼를 생성한 다음에는 반드시 Lock()과 Unlock()으로 포인터를 얻어내서 정점 정보를 정점 버퍼에 써넣어야 한다.
// 또한 D3D는 인덱스 버퍼도 사용 가능하다는 것을 명심하자.
// 정점 버퍼나 인덱스 버퍼는 기본 시스템 메모리 외에 디바이스 메모리(비디오카드 메모리)에 생성될 수 있는데,
// 대부분의 비디오카드에서는 이렇게 할 경우 엄청난 속도의 향상을 얻을 수 있다.
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
	// 삼각형을 렌더링하기 위해 3개의 정점 선언
	CUSTOMVERTEX vertices[] =
	{
		{ -1.0f, -1.0f, 0.0f, 0xffff0000, }, // x, y, z, color
		{  1.0f, -1.0f, 0.0f, 0xff0000ff, },
		{  0.0f,  1.0f, 0.0f, 0xffffffff, },
	};

	// 정점 버퍼를 생성한다.
	// 3개의 사용자 정점을 보관할 메모리를 할당한다.
	// FVF를 지정하여 보관할 데이터 형식을 지정한다.
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(
		3 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVB, NULL)))
	{
		return E_FAIL;
	}

	// 정점 버퍼를 값으로 채운다.
	// 정점 버퍼의 Lock() 함수를 호출하여 포인터를 얻어온다.
	VOID* pVertices;
	if (FAILED(g_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, vertices, sizeof(vertices));
	g_pVB->Unlock();

	return S_OK;
}

//-----------------------------------------------------------------------------
// 행렬 설정
// 행렬에는 3가지가 있으며, 각각 월드, 뷰, 프로젝션 행렬이다.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{
	// 월드 행렬
	D3DXMATRIXA16 matWorld;

	UINT iTime = timeGetTime() % 1000;							// float 연산의 정밀도를 위해서 1000으로 나머지 연산한다.
	FLOAT fAngle = iTime * (2.0f * D3DX_PI) / 1000.0f;			// 1000밀리초마다 한 바퀴씩(2 * pi) 회전 애니메이션 행렬을 만든다.
	D3DXMatrixRotationY(&matWorld, fAngle);						// Y축을 회전축으로 회전 행렬을 생성한다.
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);			// 생성한 회전 행렬을 월드 행렬로 디바이스에 설정한다.

	// 뷰 행렬을 정의하기 위해서는 3가지 값이 필요하다.
	D3DXVECTOR3 vEyePt(0.0f, 3.0f, -5.0f);						// 1. 눈의 위치(0, 3.0, -5)
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);					// 2. 눈이 바라보는 위치(0, 0, 0)
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);						// 3. 천정 방향을 나타내는 상방 벡터(0, 1, 0)
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);	// 1, 2, 3의 값으로 뷰 행렬 생성
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);		   	// 생성한 뷰 행렬을 디바이스에 설정

	// 프로젝션 행렬을 정의하기 위해서는 시야각(FOV = Field Of View)과 종횡비(aspect ratio), 클리핑 평면의 값이 필요하다.
	D3DXMATRIXA16 matProj;
	// matProj		: 값이 설정될 행렬
	// D3DX_PI / 4	: FOV(D3D_PI / 4 = 45도)
	// 1.0f			: 종횡비
	// 1.0f			: 근접 클리핑 평면(near clipping plane)
	// 100.0f		: 원거리 클리핑 평면(far clipping plane)
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);		// 생성한 프로젝션 행렬을 디바이스에 설정
}

//-----------------------------------------------------------------------------
// 초기화된 객체들을 소거한다
//-----------------------------------------------------------------------------
VOID Cleanup()
{
	if (g_pVB != NULL)
		g_pVB->Release();

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
	// 후면 버퍼를 검은색(0, 0, 0)으로 지운다.
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	// 월드, 뷰, 프로젝션 행렬을 설정한다.
	SetupMatrices();

	// 렌더링 시작
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// 정점 버퍼의 삼각형을 그린다.
		// 1. 정점 정보가 담겨 있는 정점 버퍼를 출력 스트림으로 할당한다.
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		// 2. D3D에게 정점 셰이더 정보를 지정한다. 대부분의 경우에는 FVF만 지정한다.
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		// 3. 기하 정보를 출력하기 위한 DrawPrimitive() 함수 호출
		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

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
	HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 03: Matrices",
		WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Direct3D 초기화
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		// 정점 버퍼 초기화
		if (SUCCEEDED(InitGeometry()))
		{
			// 윈도우 출력
			ShowWindow(hWnd, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);

			// 메세지 루프 진입
			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while (msg.message != WM_QUIT)
			{
				if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					// 처리할 메세지가 없으면 Render() 함수 호출
					Render();
				}
			}
		}
	}

	// 등록된 클래스 소거
	UnregisterClass("D3D Tutorial", wc.hInstance);
	return 0;
}