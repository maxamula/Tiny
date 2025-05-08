#include "application.h"
#include "graphics/renderctx.h"
#include "content/content.h"
#include "taskflow/taskflow.hpp"
#include "scene.h"
#include <functional>

// For initializing the engine
#include "engine/window_internal.h"
#include "graphics/gfxglobal.h"
#include "graphics/descriptors.h"
#include "graphics/copyqueue.h"
#include "fx/shaderfx_internal.h"
#include "content/stdmat.h"

#include "graphics/pass.h"

#include "cereal/cereal.hpp"


using namespace entt;

namespace tiny
{
	META(Window)
	{
		meta.data<&Window::hWnd>("hWnd"_hs)
			.data<&Window::width>("width"_hs)
			.data<&Window::height>("height"_hs)
			.data<&Window::vp>("vp"_hs);
	}

	bool gRunning = false;
	D3DContext gContext;
	Dispatcher gMainDispatcher;
	Dispatcher gDeferDispatcher;
	FlushDispatcher gFlushDispatcher;
	std::deque<RenderView> gRenderViews;
		
	Scene gScene;	

#pragma region Window

	Window::Window(const WindowDesc& desc)
		: hWnd(CreateWindow(TINY_WINDOW_CLASS, "Quirky Sandbox", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, GetModuleHandle("Tiny"), nullptr)),
		vp(gContext, hWnd, D3DViewportFlags_Windowed), proc(desc.proc)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		ShowWindow(hWnd, SW_SHOW);
	}

	Window::~Window()
	{
		Destroyed();
		if (hWnd)
		{
			SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
			DestroyWindow(hWnd);
			hWnd = nullptr;
		}
	}

	LRESULT CALLBACK EngineWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		/*if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
			return TRUE;*/

		if (w)
		{
			switch (message)
			{
			case WM_CLOSE:
				PostQuitMessage(0);
				break;
			case WM_SIZE:
			{
				w->width = LOWORD(lParam);
				w->height = HIWORD(lParam);
				
				gFlushDispatcher.PostOnce([w]()
				{
					w->vp.ReleaseRenderTargets();
					u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
					THROW_IF_FAILED(w->vp.swap->ResizeBuffers(w->vp.buffering, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
					w->vp.CreateRendertargetViews();
				});
				w->Resized(w->width, w->height);
			}
			}

			return w->proc ? w->proc(w, hWnd, message, wParam, lParam) : DefWindowProc(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

#pragma endregion

	Scene& GetScene()
	{
		return gScene;
	}

	class SubmitToSwapchainPass : public IRenderPass
	{
	public:
		SubmitToSwapchainPass(RenderTextureHandle targetTexture, Window* pWindow)
			: targetTexture(targetTexture), pWindow(pWindow)
		{}
		~SubmitToSwapchainPass() = default;

		void Setup(FrameGraph::Builder& builder) override
		{
			builder.Read(targetTexture);
		}

		void Execute(RenderContext& context, FrameGraphResources& resources) override
		{
			RenderTexture& tex = resources.GetRenderTexture(targetTexture);
			CComPtr<ID3D12GraphicsCommandList6> cmd = context.cmd;

			D3D12_CPU_DESCRIPTOR_HANDLE rtv = pWindow->vp.renderTargets[pWindow->vp.swap->GetCurrentBackBufferIndex()].allocation.cpu;
			cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

			float color[4] = { 0.8f, 0.8f, 0.7f, 1.0f };
			cmd->ClearRenderTargetView(rtv, color, 0, nullptr);

			D3D12_VIEWPORT viewport = { 0.0f, 0.0f, pWindow->width, pWindow->height, 0.0f, 1.0f };
			D3D12_RECT scissorRect = { 0, 0, pWindow->width, pWindow->height };
			cmd->RSSetViewports(1, &viewport);
			cmd->RSSetScissorRects(1, &scissorRect);

			TextureStretcherMaterialInstance mat;
			mat.SetFeatureFlags(0);
			mat.diffuseTexture = tex;

			context.BindMaterial(&mat);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(6, 1, 0, 0);
		}
	private:
		const Window* 				pWindow;
		const RenderTextureHandle	targetTexture;
	};

	void RunEngine()
	{
		if (gRunning)
			THROW_ENGINE_EXCEPTION("Engine is already running");
		gContext.Flush();
		gRunning = true;

		entt::entity obj = gScene.CreateObject("test object");
		gScene.mRegistry.emplace<Mesh>(obj);
		
		entt::registry renderRegistry;
		tf::Taskflow taskflow;
		tf::Task dispatchJob = taskflow.emplace([]() 
		{
				gDeferDispatcher.ProcessDispatchQueue();
				gFlushDispatcher.ProcessDispatchQueue();

				for (auto& view : gRenderViews)
				{
					if (view.bMarkedForRebuild)
					{
						view.bMarkedForRebuild = false;
						view.frameGraph.Reset();
						RenderTextureHandle renderOutput = view.renderer->Build(view.frameGraph);
						view.frameGraph.AddPass<SubmitToSwapchainPass>(renderOutput, view.window);
						view.frameGraph.Compile(gContext);
					}
				}
		}).name("Dispatch Job");
		tf::Task renderJob = taskflow.emplace([](tf::Subflow& sf)
		{
				try
				{
					gContext.BeginScene();
					
					CComPtr<ID3D12GraphicsCommandList6> cmd = gContext.mainQueue.AcquireNextCommandList();
					std::set<Window*> windows;
					
					for (auto& view : gRenderViews)
					{
						std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
						auto rt = view.window->vp.renderTargets[view.window->vp.swap->GetCurrentBackBufferIndex()];
						gContext.Defer(rt.allocation);
						gContext.Defer(rt.resource);

						ID3D12DescriptorHeap* heaps[] = { *GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), *GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
						cmd->SetDescriptorHeaps(_countof(heaps), heaps);
						
						CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt.resource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
						cmd->ResourceBarrier(1, &barrier);		
									
						view.frameGraph.ExecuteAsync(sf, gContext, gScene.mRegistry);
												
						windows.insert(view.window);
						barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
						cmd->ResourceBarrier(1, &barrier);
						
						std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
						std::chrono::duration<double, std::milli> elapsed = end - start;
						OutputDebugStringA((std::to_string(elapsed.count()) + "\n").c_str());
					}
					
					gContext.EndScene([windows]()
					{
						for (Window* window : windows)
							window->vp.Present(0);
					});
				}
				catch (const EngineException& e)
				{
					MessageBoxA(nullptr, e.what(), "Engine Exception", MB_OK | MB_ICONERROR);
					return;
				}
				catch (const std::exception& e)
				{
					MessageBoxA(nullptr, e.what(), "Exception", MB_OK | MB_ICONERROR);
					return;
				}
				catch (...)
				{
					MessageBoxA(nullptr, "Unknown exception", "Exception", MB_OK | MB_ICONERROR);
					return;
				}
				
				
		});

		dispatchJob.precede(renderJob);

		tf::Executor executor;

		executor.run_until(taskflow, [&]() { return !gRunning; });

		MSG msg;
		while (gRunning)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					gRunning = false;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			gMainDispatcher.ProcessDispatchQueue();
		}
		gContext.Flush();
	}

	RenderView* CreateRenderView(Window* pWindow, IRenderer* renderer)
	{
		RenderView& rv = gRenderViews.emplace_back(RenderView
		{
			.window = pWindow,
			.renderer = renderer,
			.bMarkedForRebuild = false,
		});

		RenderTextureHandle renderOutput = renderer->Build(rv.frameGraph);
		rv.frameGraph.AddPass<SubmitToSwapchainPass>(renderOutput, pWindow);
		rv.frameGraph.Compile(gContext);
		return &rv;
	}

	void Initialize()
	{
		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(WNDCLASSEX));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = EngineWndProc;
		wc.cbClsExtra = 0;
		wc.lpszClassName = TINY_WINDOW_CLASS;
		wc.cbWndExtra = sizeof(void*);
		wc.hInstance = GetModuleHandle("Tiny");
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&wc);

		InitializeGFX();
		InitializeGlobalDescriptorHeaps();
		InitializeGlobalCopyQueue();
		fx::InitializeFX(gDevice);

		gContext.Create(TINY_DEFAULT_BUFFERING);
	}

	void Shutdown()
	{
		gRenderViews.clear();
		gContext.Destroy();
		fx::ShutdonwnFX();
		ShutdownGlobalCopyQueue();
		ShutdownGlobalDescriptorHeaps();
		ShutdownGFX();

		UnregisterClass(TINY_WINDOW_CLASS, GetModuleHandle("Tiny"));
	}
}