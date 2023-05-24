// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ dir + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


#include <chrono>
#include <iostream>
#include <glm/gtx/normal.hpp>

#include "base.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "MeshRenderer.h"
#include "Camera.h"
#include "heightmap.h"
#include "stl.h"

#include "Texture2D.h"
#include "triangulator.h"
#include "MeshRenderer.h"
#include "PlaneRenderer.h"

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

using namespace DirectX::SimpleMath;

namespace
{
	std::unique_ptr<DirectX::Texture2D> g_depthStencil = nullptr;
	std::unique_ptr<MeshRenderer> g_MeshRenderer = nullptr;
	std::unique_ptr<PlaneRenderer> g_PlaneRenderer = nullptr;
	std::shared_ptr<PassConstants> g_Constants = nullptr;
	std::unique_ptr<Camera> g_Camera = nullptr;
}

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void CreatePipeline();
void UpdateConstants(const ImGuiIO& io);
std::pair<std::vector<Vertex>, std::vector<uint32_t>> CreateTerrainMesh(
	const std::vector<glm::vec3>& points, const std::vector<glm::ivec3>& triangles);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = {
		sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL
	};
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Renderer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL,
		NULL, wc.hInstance, NULL);

	// CreatePipeline Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
	// Load Fonts

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	CreatePipeline();

	std::string inFile = "";
	char filePath[256] {};
	const std::string outFile = "terrain.stl";
	const std::string normalmapPath = "normalMap.png"; // path to write normal map png
	const std::string shadePath = "hillShade.png"; // path to write hillshade png
	float zScale = 30.0f;	// z scale relative to x & y
	float zExaggeration = 1.0f;	// z exaggeration
	float maxError = 1.0f;	// maximum triangulation error
	int maxTriangles = 0; // maximum number of triangles
	int maxPoints = 0; // maximum number of vertices
	float baseHeight = 0; // solid base height
	bool level = true; // auto level input to full grayscale range
	bool invert = false; // invert heightmap
	int blurSigma = 0; // gaussian blur sigma
	float gamma = 0; // gamma curve exponent
	int borderSize = 0.0f; // border size in pixels
	float borderHeight = 1; // border z height
	float shadeAlt = 45; // hillshade light altitude
	float shadeAz = 0; //hillshade light azimuth
	std::string stats = "null";
	bool outputFiles = false;

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		UpdateConstants(io);

		ImGui::Begin("Height Map to Mesh");
		ImGui::InputText("input height map", filePath, 128);
		ImGui::InputFloat("z scale relative to x & y", &zScale);
		ImGui::InputFloat("z exaggeration", &zExaggeration);
		ImGui::InputFloat("maximum triangulation error * 1000", &maxError);
		ImGui::InputInt("maximum number of triangles", &maxTriangles);
		ImGui::InputInt("maximum number of vertices", &maxPoints);
		ImGui::InputFloat("solid base height", &baseHeight);
		ImGui::Checkbox("auto level input to full grayscale range", &level);
		ImGui::Checkbox("invert heightmap", &invert);
		ImGui::InputInt("gaussian blur sigma", &blurSigma);
		ImGui::InputFloat("gamma curve exponent", &gamma);
		ImGui::InputInt("border size in pixels", &borderSize);
		ImGui::InputFloat("border z height", &borderHeight);
		ImGui::Checkbox("output hillshade and normal", &outputFiles);
		ImGui::InputFloat("hillshade light altitude", &shadeAlt);
		ImGui::InputFloat("hillshade light azimuth", &shadeAz);
		bool run = ImGui::Button("TRIANGULATE");
		ImGui::Text(stats.c_str());
		ImGui::End();

		if (run)
		{
			inFile = filePath;
			const auto startTime = std::chrono::steady_clock::now();
			// helper function to display elapsed time of each step
			const auto timed = [](const std::string& message) -> std::function<void()>
			{
				printf("%s... ", message.c_str());
				fflush(stdout);
				const auto startTime = std::chrono::steady_clock::now();
				return [message, startTime]()
				{
					const std::chrono::duration<double> elapsed =
						std::chrono::steady_clock::now() - startTime;
					printf("%gs\n", elapsed.count());
				};
			};

			// load heightmap
			auto done = timed("loading heightmap");
			const auto hm = std::make_shared<Heightmap>(inFile);
			done();

			int w = hm->Width();
			int h = hm->Height();
			if (w * h == 0)
			{
				std::cerr << "invalid heightmap file (try png, jpg, etc.)" << std::endl;
				std::exit(1);
			}

			printf("  %d x %d = %d pixels\n", w, h, w * h);

			// auto level heightmap
			if (level)
			{
				hm->AutoLevel();
			}

			// invert heightmap
			if (invert)
			{
				hm->Invert();
			}

			// blur heightmap
			if (blurSigma > 0)
			{
				done = timed("blurring heightmap");
				hm->GaussianBlur(blurSigma);
				done();
			}

			// apply gamma curve
			if (gamma > 0)
			{
				hm->GammaCurve(gamma);
			}

			// add border
			if (borderSize > 0)
			{
				hm->AddBorder(borderSize, borderHeight);
			}

			// get updated size
			w = hm->Width();
			h = hm->Height();

			// triangulate
			done = timed("triangulating");
			Triangulator tri(hm);
			tri.Run(maxError / 1000.0f, maxTriangles, maxPoints);
			auto points = tri.Points(zScale * zExaggeration);
			auto triangles = tri.Triangles();
			done();

			const auto& [vb, ib] = CreateTerrainMesh(points, triangles);
			g_MeshRenderer->SetVerticesAndIndices(vb, ib);

			// add base
			if (baseHeight > 0)
			{
				done = timed("adding solid base");
				const float z = -baseHeight * zScale * zExaggeration;
				AddBase(points, triangles, w, h, z);
				done();
			}

			// display statistics
			const int naiveTriangleCount = (w - 1) * (h - 1) * 2;
			printf("  error = %g\n", tri.Error());
			printf("  points = %ld\n", points.size());
			printf("  triangles = %ld\n", triangles.size());
			printf("  vs. naive = %g%%\n", 100.f * triangles.size() / naiveTriangleCount);
			stats =
				std::to_string(triangles.size()) + " triangles" + "\n" +
				std::to_string(points.size()) + " vertices" + "\n" +
				std::to_string(tri.Error()) + " error" + "\n" +
				std::to_string(100.f * triangles.size() / naiveTriangleCount) + "% vs. naive\n";

			if (outputFiles)
			{
				// write output file
				done = timed("writing output");
				SaveBinarySTL(outFile, points, triangles);
				done();

				// compute normal map
				done = timed("computing normal map");
				hm->SaveNormalmap(normalmapPath, zScale * zExaggeration);
				done();

				// compute hillshade image
				done = timed("computing hillshade image");
				hm->SaveHillshade(shadePath, zScale * zExaggeration, shadeAlt, shadeAz);
				done();
			}

			// show total elapsed time
			const std::chrono::duration<double> elapsed =
				std::chrono::steady_clock::now() - startTime;
			printf("%gs\n", elapsed.count());
		}

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = {
			clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w
		};
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_depthStencil->GetDsv());
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		g_pd3dDeviceContext->ClearDepthStencilView(g_depthStencil->GetDsv(),
			D3D11_CLEAR_DEPTH, 1.0f, 0);

		g_Camera->SetViewPort(g_pd3dDeviceContext);
		g_MeshRenderer->Render(g_pd3dDeviceContext);
		//g_PlaneRenderer->UpdateBuffer(g_pd3dDeviceContext);
		//g_PlaneRenderer->Render(g_pd3dDeviceContext);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		//g_pSwapChain->Present(1, 0); // Present with vsync
		g_pSwapChain->Present(0, 0); // Present without vsync
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
		&g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
		g_pSwapChain = NULL;
	}
	if (g_pd3dDeviceContext)
	{
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = NULL;
	}
	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);

	D3D11_TEXTURE2D_DESC texDesc;
	pBackBuffer->GetDesc(&texDesc);
	CD3D11_TEXTURE2D_DESC dsDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,
		texDesc.Width, texDesc.Height, 1, 0, D3D11_BIND_DEPTH_STENCIL,
		D3D11_USAGE_DEFAULT);
	g_depthStencil = std::make_unique<DirectX::Texture2D>(g_pd3dDevice, dsDesc);
	g_depthStencil->CreateViews(g_pd3dDevice);

	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView)
	{
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = NULL;
	}
}

void CreatePipeline()
{
	g_Constants = std::make_unique<PassConstants>();
	g_MeshRenderer = std::make_unique<MeshRenderer>(g_pd3dDevice, g_Constants);
	g_MeshRenderer->Initialize(g_pd3dDeviceContext);
	g_PlaneRenderer = std::make_unique<PlaneRenderer>(g_pd3dDevice, g_Constants);
	g_PlaneRenderer->Initialize(g_pd3dDeviceContext);
	g_Camera = std::make_unique<Camera>();
}

void UpdateConstants(const ImGuiIO& io)
{
	g_Camera->Update(io);
	g_Constants->ViewProjection = g_Camera->GetViewProjectionMatrix().Transpose();
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> CreateTerrainMesh(
	const std::vector<glm::vec3>& points, const std::vector<glm::ivec3>& triangles)
{
	std::vector<Vertex> vertices;
	vertices.reserve(points.size());
	for (const auto& point : points)
		vertices.emplace_back(Vector3(point.x, point.z, point.y));

	std::vector<uint32_t> indices;
	indices.reserve(triangles.size() * 3);
	for (const auto& triangle : triangles)
	{
		indices.emplace_back(triangle.x);
		indices.emplace_back(triangle.y);
		indices.emplace_back(triangle.z);
	}

	return { vertices, indices };
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE: if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY: ::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
