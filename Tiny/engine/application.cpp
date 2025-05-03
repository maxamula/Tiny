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

#include "content/stdpass.h" // TODO
#include "graphics/pass.h"

#include <chrono>

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

	struct TINYFX_API RenderView
	{
	_internal:
		Window* window;
		IRenderer* renderer;
		D3D12_VIEWPORT viewport;
	};

	bool gRunning = false;
	D3DContext gContext;
	Dispatcher gMainDispatcher;
	Dispatcher gDeferDispatcher;
	FlushDispatcher gFlushDispatcher;
	std::deque<RenderView> gRenderViews;
	
	//FSSamplerMaterialInstance gFFSamplerSwapchainMat;
	BeamMaterialInstance gBeamMat;
	
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
			}
			}

			return w->proc ? w->proc(w, hWnd, message, wParam, lParam) : DefWindowProc(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

#pragma endregion

	void LoadScene(const std::string& path)
	{
		gDeferDispatcher.post([path]()
		{
			gScene = Scene();
			entt::entity obj = gScene.CreateObject("test object");
			//gScene.registry.emplace<Mesh>(obj);
		});
	}

	class SubmitToSwapchainPass : public IRenderPass
	{
	public:
		SubmitToSwapchainPass() = default;
		~SubmitToSwapchainPass() = default;

		void Execute(RenderContext& context, FrameGraphResources& resources) override
		{
			RenderTexture& tex = resources.GetRenderTexture(finalOutput);
			CComPtr<ID3D12GraphicsCommandList6> cmd = context.cmd;

			D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapchainTarget->allocation.cpu;
			cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

			float color[4] = { 0.8f, 0.8f, 0.7f, 1.0f };
			cmd->ClearRenderTargetView(rtv, color, 0, nullptr);

			D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 1264, 681, 0.0f, 1.0f };
			D3D12_RECT scissorRect = { 0, 0, 1264, 681 };
			cmd->RSSetViewports(1, &viewport);
			cmd->RSSetScissorRects(1, &scissorRect);

			TextureStretcherMaterialInstance mat;
			mat.SetFeatureFlags(0);
			mat.diffuseTexture = tex;

			context.BindMaterial(&mat);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(6, 1, 0, 0);
		}

		D3DViewport::RenderTarget*	swapchainTarget;
		RenderTextureHandle			finalOutput;
	};

	void RunEngine()
	{
		if (gRunning)
			THROW_ENGINE_EXCEPTION("Engine is already running");
		gContext.Flush();
		gBeamMat.SetFeatureFlags(0);
		gRunning = true;
		
		entt::registry renderRegistry;
		tf::Taskflow taskflow;
		tf::Task dispatchJob = taskflow.emplace([]() {gDeferDispatcher.ProcessDispatchQueue(); gFlushDispatcher.ProcessDispatchQueue();}).name("Dispatch Job");
		tf::Task renderJob = taskflow.emplace([]()
		{
				
				try
				{
					gContext.BeginScene();
					CComPtr<ID3D12GraphicsCommandList6> cmd = gContext.mainQueue.GetCurrentCommandList();
					std::set<Window*> windows;
					
					for (auto& view : gRenderViews)
					{
						auto rt = view.window->vp.renderTargets[view.window->vp.swap->GetCurrentBackBufferIndex()];
						gContext.Defer(rt.allocation);
						gContext.Defer(rt.resource);

						RenderContext renderContext
						{
							.context = gContext,
							.renderItems = &gScene.registry,
							.cmd = cmd
						};
						// set descriptor heaps
						ID3D12DescriptorHeap* heaps[] = { *GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), *GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
						cmd->SetDescriptorHeaps(_countof(heaps), heaps);

						CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt.resource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
						cmd->ResourceBarrier(1, &barrier);

						// Set render target and scissor rect and viewport
						D3D12_CPU_DESCRIPTOR_HANDLE rtv = rt.allocation.cpu;
						FrameGraph frameGraph;
						RenderTextureHandle finalOutput = view.renderer->Build(frameGraph);

						SubmitToSwapchainPass* swapchainPass = frameGraph.AddPass<SubmitToSwapchainPass>(
						[&](FrameGraph::Builder& builder, SubmitToSwapchainPass& pass)
						{
							pass.finalOutput = finalOutput;
							pass.swapchainTarget = &rt;
							builder.Read(pass.finalOutput);
						});

						
						frameGraph.Compile();
						
						frameGraph.Execute(renderContext, gContext);
						
						
						/*BeamMaterialInstance beamMat;
						beamMat.SetFeatureFlags(0);
						beamMat.ilShaderParams.data.width = view.window->width;
						beamMat.ilShaderParams.data.height = view.window->height;
						static float time = 0.0f;
						beamMat.ilShaderParams.data.time = time;
						time += 0.001f;*/

						//// Set triangle list
						/*renderContext.BindMaterial(&beamMat);
						cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						cmd->DrawInstanced(3, 1, 0, 0);*/


						windows.insert(view.window);
						barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
						cmd->ResourceBarrier(1, &barrier);
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

	void CreateRenderView(Window* pWindow, IRenderer* renderer)
	{
		gDeferDispatcher.post([=]()
		{
			RenderView& rv = gRenderViews.emplace_back(RenderView
			{
				.window = pWindow,
				.renderer = renderer,
				.viewport =
				{
					0.0f, 0.0f,
					static_cast<FLOAT>(pWindow->width),
					static_cast<FLOAT>(pWindow->height),
					0.0f, 1.0f
				},
			});			
		});
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