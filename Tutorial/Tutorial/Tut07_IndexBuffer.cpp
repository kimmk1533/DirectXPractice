//-----------------------------------------------------------------------------
// 파일:	Tut07_IndexBuffer.cpp
//
// 설명:	인덱스 버퍼(Index Buffer)란 정점을 보관하기 위한 정점 버퍼(VB) 처럼
//		인덱스를 보관하기 위한 전용 객체다. D3D 학습 예제에는 이렇게 IB를 사용한
//		예제가 없기 때문에 새롭게 추가시킨 것이다.
//
//	Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#pragma warning(disable: 28251)	// WinMain 주석 오류 경고
#pragma warning(disable: 6031)	// 반환값 무시 오류 경고

//-----------------------------------------------------------------------------
// 전역 변수
//-----------------------------------------------------------------------------
LPDIRECT3D9				g_pD3D = NULL;				// D3D 디바이스를 생성할 D3D 객체 변수
LPDIRECT3DDEVICE9		g_pd3dDevice = NULL;		// 렌더링에 사용될 D3D 디바이스
LPDIRECT3DVERTEXBUFFER9	g_pVB = NULL;				// 정점을 보관할 정점 버퍼
LPDIRECT3DINDEXBUFFER9	g_pIB = NULL;				// 인덱스를 보관할 인덱스 버퍼

// 사용자 정점을 정의할 구조체
struct CUSTOMVERTEX
{
	FLOAT x, y, z;		// 정점의 변환된 좌표
	DWORD color;		// 정점의 색깔
};

// 사용자 정점 구조체에 관한 정보를 나타내는 FVF 값
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

struct MYINDEX
{
	WORD _0, _1, _2;		// 일반적으로 인덱스는 16비트의 크기를 갖는다.
							// 32비트의 크기도 가능하지만 구형 그래픽카드(TNT급, 지금은 해당X)에서는 지원되지 않는다.
};

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
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

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

	// 컬링 기능을 끈다.
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	// Z 버퍼 기능을 켠다.
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

	// 정점에 색깔값이 있으므로 광원 기능을 끈다.
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	return S_OK;
}

//-----------------------------------------------------------------------------
// 정점 버퍼를 생성하고 정점값을 채워넣는다.
// 정점 버퍼란 기본적으로 정점 정보를 갖고 있는 메모리 블록이다.
// 정점 버퍼를 생성한 다음에는 반드시 Lock()과 Unlock()으로 포인터를 얻어내서 정점 정보를 정점 버퍼에 써넣어야 한다.
// 또한 D3D는 인덱스 버퍼도 사용 가능하다는 것을 명심하자.
// 정점 버퍼나 인덱스 버퍼는 기본 시스템 메모리 외에 디바이스 메모리(비디오카드 메모리)에 생성될 수 있는데,
// 대부분의 비디오카드에서는 이렇게 할 경우 엄청난 속도의 향상을 얻을 수 있다.
//-----------------------------------------------------------------------------
HRESULT InitVB()
{
	// 상자(Cube)를 렌더링하기 위해 8개의 정점 선언
	CUSTOMVERTEX vertices[] =
	{
		{ -1,  1,  1, 0xffff0000 },			// v0
		{  1,  1,  1, 0xff00ff00 },			// v1
		{  1,  1, -1, 0xff0000ff },			// v2
		{ -1,  1, -1, 0xffffff00 },			// v3

		{ -1, -1,  1, 0xff00ffff },			// v4
		{  1, -1,  1, 0xffff00ff },			// v5
		{  1, -1, -1, 0xff000000 },			// v6
		{ -1, -1, -1, 0xffffffff },			// v7
	};

	// 정점 버퍼 생성
	// 8개의 사용자 정점을 보관할 메모리를 할당한다.
	// FVF를 지정하여 보관할 데이터의 형식을 지정한다.
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(8 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB, NULL)))
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

HRESULT InitIB()
{
	// 상자(Cube)를 렌더링하기 위한 12개의 면 선언
	MYINDEX indices[] =
	{
		{ 0, 1, 2 }, { 0, 2, 3 },		// 윗면
		{ 4, 6, 5 }, { 4, 7, 6 },		// 아랫면
		{ 0, 3, 7 }, { 0, 7, 4 },		// 왼면
		{ 1, 5, 6 }, { 1, 6, 2 },		// 오른면
		{ 3, 2, 6 }, { 3, 6, 7 },		// 앞면
		{ 0, 4, 5 }, { 0, 5, 1 },		// 뒷면
	};

	// 인덱스 버퍼 생성
	// D3DFMT_INDEX16은 인덱스의 단위가 16비트라는 것이다.
	// 우리는 MYINDEX 구조체에서 WORD형으로 선언했으므로 D3DFMT_INDEX16을 사용한다.
	if (FAILED(g_pd3dDevice->CreateIndexBuffer(12 * sizeof(MYINDEX),
		0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pIB, NULL)))
	{
		return E_FAIL;
	}

	// 인덱스 버퍼를 값으로 채운다.
	// 인덱스 버퍼의 Lock() 함수를 호출하여 포인터를 얻어온다.
	VOID* pIndices;
	if (FAILED(g_pIB->Lock(0, sizeof(indices), (void**)&pIndices, 0)))
		return E_FAIL;
	memcpy(pIndices, indices, sizeof(indices));
	g_pIB->Unlock();

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
	D3DXMatrixIdentity(&matWorld);								// 월드 행렬을 단위 행렬로 생성
	D3DXMatrixRotationY(&matWorld, timeGetTime() / 500.0f);		// Y축을 중심으로 회전 행렬 생성
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);			// 디바이스에 월드 행렬 설정

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
	if (g_pIB != NULL)
		g_pIB->Release();

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
	// 후면 버퍼와 Z 버퍼를 지운다
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

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
		// 3. 인덱스 버퍼를 지정한다.
		g_pd3dDevice->SetIndices(g_pIB);
		// 4. DrawIndexedPrimitive()를 호출한다.
		g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

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
	HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 07: IndexBuffer",
		WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Direct3D 초기화
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		// 정점 버퍼 초기화
		if (SUCCEEDED(InitVB()))
		{
			// 인덱스 버퍼 초기화
			if (SUCCEEDED(InitIB()))
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
	}

	// 등록된 클래스 소거
	UnregisterClass("D3D Tutorial", wc.hInstance);
	return 0;
}