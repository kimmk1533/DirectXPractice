//-----------------------------------------------------------------------------
// 파일:	Tut06_Meshes.cpp
//
// 설명:	보다 멋진 기하 정보를 출력하기 위해서는 전문적인 3D 모델러가 만든 모델을 파일로
//		읽어들이는 것이 일반적이다. 다행스럽게도 D3DX에는 강력한 X파일 처리 기능이 있어서
//		정점 버퍼, 인덱스 버퍼 생성 등의 많은 부분을 대신해준다. 이 예제에서는 D3DMESH를
//		사용하여 파일을 읽어서 이 파일과 연관된 재질과 텍스처를 함께 사용하는 것을 알아보자.
//
//		이번에 소개되지는 않지만 나중에 사용하게 될 강력한 기능 중에 하나가 FVF를 지정하여
//		새로운 메시를 복제(clone)하는 것이다. 이 기능을 사용하여 텍스처 좌표나 법선 벡터 등을
//		기존 메시에 추가한 새로운 메시를 생성할 수 있다.
//		(나중에 Cg, HLSL 등을 공부할 때 많이 사용될 것이다).
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
#pragma warning(disable: 6031)	// 반환값 무시 오류 경고

//-----------------------------------------------------------------------------
// 전역 변수
//-----------------------------------------------------------------------------
LPDIRECT3D9				g_pD3D = NULL;				// D3D 디바이스를 생성할 D3D 객체 변수
LPDIRECT3DDEVICE9		g_pd3dDevice = NULL;		// 렌더링에 사용될 D3D 디바이스

LPD3DXMESH				g_pMesh = NULL;				// 메시 객체
D3DMATERIAL9*			g_pMeshMaterials = NULL;	// 메시에서 사용할 재질
LPDIRECT3DTEXTURE9*		g_pMeshTextures = NULL;		// 메시에서 사용할 텍스처
DWORD					g_dwNumMaterials = 0L;		// 메시에서 사용중인 재질의 개수

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

	// Z 버퍼 기능을 켠다.
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

	// 주변 광원 값을 최대 밝기로
	g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0xffffffff);

	return S_OK;
}

//-----------------------------------------------------------------------------
// 기하 정보 초기화
// 메시 읽기, 재질과 텍스처 배열 생성
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
	// 재질을 임시로 보관할 버퍼 선언
	LPD3DXBUFFER pD3DMtrlBuffer;

	// Tiger.x 파일을 메시로 읽어들인다. 이 때 재질 정보도 함께 읽는다.
	if (FAILED(D3DXLoadMeshFromX("Tiger.x", D3DXMESH_SYSTEMMEM,
		g_pd3dDevice, NULL, &pD3DMtrlBuffer, NULL,
		&g_dwNumMaterials, &g_pMesh)))
	{
		// 현재 폴더에 파일이 없으면 상위 폴더 검색
		if (FAILED(D3DXLoadMeshFromX("..\\Tiger.x", D3DXMESH_SYSTEMMEM,
			g_pd3dDevice, NULL, &pD3DMtrlBuffer, NULL,
			&g_dwNumMaterials, &g_pMesh)))
		{
			MessageBox(NULL, "Could not find tiger.x", "Meshes.exe", MB_OK);

			return E_FAIL;
		}
	}

	// 재질 정보와 텍스처 정보를 따로 뽑아낸다.
	D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DMtrlBuffer->GetBufferPointer();
	g_pMeshMaterials = new D3DMATERIAL9[g_dwNumMaterials];		// 재질 개수만큼 재질 구조체 배열 생성
	g_pMeshTextures = new LPDIRECT3DTEXTURE9[g_dwNumMaterials];	// 재질 개수만큼 텍스처 배열 생성

	for (DWORD i = 0; i < g_dwNumMaterials; ++i)
	{
		// 재질 정보 복사
		g_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;

		// 주변 광원 정보를 Diffuse 정보로
		g_pMeshMaterials[i].Ambient = g_pMeshMaterials[i].Diffuse;

		g_pMeshTextures[i] = NULL;
		if (d3dxMaterials[i].pTextureFilename != NULL &&
			lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
		{
			// 텍스처를 파일에서 로드한다.
			if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
				d3dxMaterials[i].pTextureFilename, &g_pMeshTextures[i])))
			{
				// 텍스처가 현재 폴더에 없으면 상위 폴더 검색
				const TCHAR* strPrefix = TEXT("..\\");
				const int  lenPrefix = lstrlen(strPrefix);
				TCHAR strTexture[MAX_PATH];
				lstrcpyn(strTexture, strPrefix, MAX_PATH);
				lstrcpyn(strTexture + lenPrefix,
					d3dxMaterials[i].pTextureFilename, MAX_PATH - lenPrefix);
				if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
					strTexture, &g_pMeshTextures[i])))
				{
					MessageBox(NULL, "Could not find texture map", "Meshes.exe", MB_OK);
				}
			}
		}
	}

	// 임시로 생성한 재질 버퍼 소거
	pD3DMtrlBuffer->Release();

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
	D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);	// Y축을 중심으로 회전 행렬 생성
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
	if (g_pMeshMaterials != NULL)
		delete[] g_pMeshMaterials;

	if (g_pMeshTextures)
	{
		for (DWORD i = 0; i < g_dwNumMaterials; ++i)
		{
			if (g_pMeshTextures[i])
				g_pMeshTextures[i]->Release();
		}
		delete[] g_pMeshTextures;
	}

	if (g_pMesh != NULL)
		g_pMesh->Release();

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
		// 메시는 재질이 다른 메시별로 부분 집합을 이루고 있다.
		// 이들을 루프를 수행해서 모두 그려준다.
		for (DWORD i = 0; i < g_dwNumMaterials; ++i)
		{
			// 부분 집합 메시의 재질과 텍스처 생성
			g_pd3dDevice->SetMaterial(&g_pMeshMaterials[i]);
			g_pd3dDevice->SetTexture(0, g_pMeshTextures[i]);

			// 부분 집합 메시 출력
			g_pMesh->DrawSubset(i);
		}
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
	HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 06: Meshes",
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