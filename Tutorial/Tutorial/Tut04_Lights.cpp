//-----------------------------------------------------------------------------
// 파일:	Tut04_Lights.cpp
//
// 설명:	조명을 사용하면 훨씬 더 극적인 연출이 가능하다.
//		조명을 사용하기 위해서는 광원이나 재질을 생성해야 한다.
//		또한 기하 정보에 법선 벡터 정보를 포함하고 있어야 한다.
//		광원은 위치, 색깔, 방향 등의 정보를 바탕으로 생성된다.
//
//	Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>

#pragma warning(disable: 28251)	// WinMain 주석 오류 경고

//-----------------------------------------------------------------------------
// 전역 변수
//-----------------------------------------------------------------------------
LPDIRECT3D9				g_pD3D = NULL;			// D3D 디바이스를 생성할 D3D 객체 변수
LPDIRECT3DDEVICE9		g_pd3dDevice = NULL;	// 렌더링에 사용될 D3D 디바이스
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;			// 정점을 보관할 정점 버퍼

// 사용자 정점을 정의할 구조체
// 광원을 사용하기 때문에 법선 벡터가 있어야 한다는 사실을 명심하자.
struct CUSTOMVERTEX
{
	D3DXVECTOR3 position;	// 정점의 3차원 좌표
	D3DXVECTOR3 normal;		// 정점의 법선 벡터
};

// 사용자 정점 구조체에 관한 정보를 나타내는 FVF 값
// 구조체는 X, Y, Z 값과 Diffuse 색깔 값으로 이루어져 있음을 알 수 있다.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL)

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

	// 컬링 기능을 끈다. 삼각형의 앞면, 뒷면을 모두 렌더링한다
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// Z 버퍼 기능을 켠다.
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

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
	// 정점 버퍼를 생성한다.
	// FVF를 지정하여 보관할 데이터 형식을 지정한다.
	if (FAILED(g_pd3dDevice->CreateVertexBuffer
	(
		50 * 2 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVB, NULL
	)))
	{
		return E_FAIL;
	}

	// 알고리즘을 사용하여 실린더(위와 아래가 뚫린 원통)를 만든다.
	CUSTOMVERTEX* pVertices;
	if (FAILED(g_pVB->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	for (DWORD i = 0; i < 50; ++i)
	{
		FLOAT theta = (2 * D3DX_PI * i) / (50 - 1);
		pVertices[2 * i + 0].position = D3DXVECTOR3(sinf(theta), -1.0f, cosf(theta));	// 실린더의 아래쪽 원통의 좌표
		pVertices[2 * i + 0].normal = D3DXVECTOR3(sinf(theta), 0.0f, cosf(theta));		// 실린더의 아래쪽 원통의 법선 벡터
		pVertices[2 * i + 1].position = D3DXVECTOR3(sinf(theta), 1.0f, cosf(theta));	// 실린더의 　위쪽 원통의 좌표
		pVertices[2 * i + 1].normal = D3DXVECTOR3(sinf(theta), 0.0f, cosf(theta));		// 실린더의 　위쪽 원통의 법선 벡터
	}
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
	D3DXMatrixIdentity(&matWorld);								// 월드 행렬을 단위 행렬로 생성
	D3DXMatrixRotationX(&matWorld, timeGetTime() / 500.0f);		// X축을 중심으로 회전 행렬 생성
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
// 광원 설정
//-----------------------------------------------------------------------------
VOID SetupLights()
{
	// 재질(material) 설정
	// 재질은 디바이스에 단 하나만 설정될 수 있다.
	D3DMATERIAL9 mtrl;
	ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
	mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
	mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
	mtrl.Diffuse.b = mtrl.Ambient.b = 0.0f;
	mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
	g_pd3dDevice->SetMaterial(&mtrl);

	// 광원 설정
	D3DXVECTOR3 vecDir;						// 방향성 광원(directional light)이 향할 빛의 방향
	D3DLIGHT9 light;						// 광원 구조체
	ZeroMemory(&light, sizeof(D3DLIGHT9));	// 구조체를 0으로 지운다.
	light.Type = D3DLIGHT_DIRECTIONAL;		// 광원의 종류(점 광원, 방향성 광원, 점적 광원)
	// 광원의 색깔과 밝기
	light.Diffuse.r = 1.0f;
	light.Diffuse.g = 1.0f;
	light.Diffuse.b = 1.0f;
	// 광원의 방향
	vecDir = D3DXVECTOR3(cosf(timeGetTime() / 350.0f), 1.0f, sinf(timeGetTime() / 350.0f));

	D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);		// 광원의 방향을 단위 벡터로 만든다.
	light.Range = 1000.0f;											// 광원이 다다를 수 있는 최대 거리
	g_pd3dDevice->SetLight(0, &light);								// 디바이스에 0번 광원 설치
	g_pd3dDevice->LightEnable(0, TRUE);								// 0번 광원을 켠다.

	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);				// 광원 설정을 켠다.

	g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00202020);		// 환경 광원(ambient light)의 값 설정
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
	// 후면 버퍼와 Z 버퍼를 지운다
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

	// 광원과 재질 설정
	SetupLights();

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
		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2 * 50 - 2);

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
	HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 04: Lights",
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