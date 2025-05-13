#include "application.h"
#include "graphics/renderctx.h"
#include "content/content.h"
#include "taskflow/taskflow.hpp"
#include "taskflow/algorithm/for_each.hpp"
#include "scene.h"
#include <functional>
#include <windowsx.h>

// For initializing the engine
#include "engine/window_internal.h"
#include "graphics/gfxglobal.h"
#include "graphics/descriptors.h"
#include "graphics/copyqueue.h"
#include "fx/shaderfx_internal.h"
#include "content/stdmat.h"

#include "graphics/pass.h"
#include "graphics/imgui/imgui.h"
#include "graphics/imgui/imgui_impl_win32.h"
#include "graphics/imgui/imgui_impl_dx12.h"

using namespace entt;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(ImGuiContext* ctx, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace tiny
{
	META(Window)
	{
		meta.data<&Window::hWnd>("hWnd"_hs)
			.data<&Window::width>("width"_hs)
			.data<&Window::height>("height"_hs)
			.data<&Window::vp>("vp"_hs);
	}

	struct LockedBus
	{
		entt::dispatcher dispatcher;
		std::mutex mutex;
	};

	bool					gRunning = false;
	bool					gDebugGraphics = false;
	D3DContext				gContext;
	Dispatcher				gMainDispatcher;
	Dispatcher				gDeferDispatcher;
	std::deque<RenderView>	gRenderViews;
	std::deque<Scene>		gScenes;
	std::deque<LockedBus>	gSceneDispatchers;
	std::mutex				gResizeMutex;
	std::set<Window*>		gNeedResizeWindows;	

#pragma region Window

	Window::Window(const WindowDesc& desc)
		: hWnd(CreateWindow(TINY_WINDOW_CLASS, desc.title.c_str(),
			desc.hParent ? WS_CHILD : WS_OVERLAPPEDWINDOW, desc.posX, desc.posY, desc.width, desc.height, desc.hParent, nullptr, GetModuleHandle("Tiny"), nullptr)),
		vp(gContext, hWnd, D3DViewportFlags_Windowed), proc(desc.proc), mImContext(nullptr), mDispatcher(gSceneDispatchers.emplace_back())
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

		if (gDebugGraphics)
		{
			mImguiSrv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).AllocateDescriptor();

			mImContext = ImGui::CreateContext();
			ImGui::StyleColorsDark(mImContext);
			ImGuiIO& io = ImGui::GetIO(mImContext); (void)io;
			ImGui_ImplWin32_Init(mImContext, hWnd);
			ImGui_ImplDX12_Init(mImContext, gDevice, gContext.framesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM,
				*GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
				mImguiSrv.cpu, mImguiSrv.gpu);
		}
	}

	Window::~Window()
	{
		if (mImContext)
		{
			ImGui_ImplDX12_Shutdown(mImContext);
			ImGui_ImplWin32_Shutdown(mImContext);
			ImGui::DestroyContext(mImContext);
		}

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

		if (w)
		{
			if (gDebugGraphics && w->mImContext && ImGui_ImplWin32_WndProcHandler(w->mImContext, hWnd, message, wParam, lParam))
				return TRUE;

			switch (message)
			{
			case WM_CLOSE:
				PostQuitMessage(0);
				break;
			case WM_SIZE:
			{
				w->width = LOWORD(lParam);
				w->height = HIWORD(lParam);
				{
					std::lock_guard lock(gResizeMutex);
					gNeedResizeWindows.insert(w);
				}
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				std::lock_guard lock(w->mDispatcher.mutex);
				w->mDispatcher.dispatcher.enqueue<KeyEvent>(u16(wParam), true, ((lParam & 0x40000000) != 0));
			}
			break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				std::lock_guard lock(w->mDispatcher.mutex);
				w->mDispatcher.dispatcher.enqueue<KeyEvent>(u16(wParam), false, false);
			}
			break;	
			case WM_MOUSEMOVE:
			{
				std::lock_guard lock(w->mDispatcher.mutex);
				static POINT last; 
				POINT cur
				{ 
					.x = GET_X_LPARAM(lParam),
					.y = GET_Y_LPARAM(lParam)
				};
				w->mDispatcher.dispatcher.enqueue<MouseMoveEvent>(f32(cur.x - last.x), f32(cur.y - last.y));
				last = cur;
			}
			break;
			case WM_MOUSEWHEEL:
			{
				std::lock_guard lock(w->mDispatcher.mutex);
				w->mDispatcher.dispatcher.enqueue<MouseWheelEvent>(GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f);
			}
			break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			{
				std::lock_guard lock(w->mDispatcher.mutex);
				MouseButtonEvent::MouseButton button = MouseButtonEvent::MouseButton_Left;
				if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
					button = MouseButtonEvent::MouseButton_Right;
				else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP)
					button = MouseButtonEvent::MouseButton_Middle;
				w->mDispatcher.dispatcher.enqueue<MouseButtonEvent>(button, message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN);
			}
			break;
			}

			return w->proc ? w->proc(w, hWnd, message, wParam, lParam) : DefWindowProc(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	entt::dispatcher& Window::InputBus()
	{
		return mDispatcher.dispatcher;
	}

	void Window::Show()
	{
		gMainDispatcher.Post([this]()
		{
			ShowWindow(hWnd, SW_SHOW);
		});
	}

	void Window::Hide()
	{
		gMainDispatcher.Post([this]()
		{
			ShowWindow(hWnd, SW_HIDE);
		});
	}

	void Window::SetTitle(const std::string& title)
	{
		gMainDispatcher.Post([this, title]()
			{
				SetWindowTextA(hWnd, title.c_str());
			});
	}

	void Window::SetFullscreen(bool fullscreen)
	{
		vp.swap->SetFullscreenState(fullscreen, nullptr);
	}

	void Window::SetRect(u16 x, u16 y, u16 width, u16 height)
	{
		gMainDispatcher.Post([this, x, y, width, height]()
		{
			SetWindowPos(hWnd, HWND_TOP, x, y, width, height, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		});
	}

#pragma endregion

	class SubmitToSwapchainPass : public RenderPassBase
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

			float color[4] = { 0.01f, 0.01f, 0.01f, 1.0f };
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

			if (gDebugGraphics && pWindow->mImContext)
			{
				ImGuiContext* imctx = pWindow->mImContext;
				ImGui_ImplDX12_NewFrame(imctx);
				ImGui_ImplWin32_NewFrame(imctx);
				ImGui::NewFrame(imctx);

				ImGui::SetNextWindowPos(imctx, ImVec2(static_cast<f32>(pWindow->width) - 300.0, 0), ImGuiCond_Always);
				ImGui::SetNextWindowSize(imctx, ImVec2(300.0f, static_cast<f32>(pWindow->height)), ImGuiCond_Always);
				ImGui::Begin(imctx, "TINY DEBUGGER", nullptr, ImGuiWindowFlags_NoResize);
				ImGui::End(imctx);

				ImGui::Render(imctx);
				ImGui_ImplDX12_RenderDrawData(imctx, ImGui::GetDrawData(imctx), context.cmd);
			}
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
	
		tf::Taskflow taskflow;
		tf::Task dispatchJob = taskflow.emplace([]() 
		{
				gDeferDispatcher.ProcessDispatchQueue();	
				{
					std::lock_guard lock(gResizeMutex);
					if (!gNeedResizeWindows.empty())
					{
						gContext.Flush();
						for (Window* window : gNeedResizeWindows)
						{
							window->vp.ReleaseRenderTargets();
							u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
							THROW_IF_FAILED(window->vp.swap->ResizeBuffers(window->vp.buffering, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
							window->vp.CreateRendertargetViews();
						}
						gNeedResizeWindows.clear();
					}
				}

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

		tf::Task logicJob = taskflow.emplace([]()
		{
				for (auto& dispatcher : gSceneDispatchers)
				{
					std::lock_guard lock(dispatcher.mutex);
					dispatcher.dispatcher.update();
				}
				for (auto& scene : gScenes)
				{
					auto view = scene.mRegistry.view<ComponentScript>();
					for (auto [entity, script] : view.each())
					{
						script.script->OnUpdate(entity, 0.001);
					}
				}
		}).name("Logic Job");

		tf::Task renderJob = taskflow.emplace([](tf::Subflow& sf)
		{
				try
				{
					gContext.BeginScene();
					std::set<Window*> windows;
					for (auto& view : gRenderViews)
					{
						D3DViewport::RenderTarget rt = view.window->vp.renderTargets[view.window->vp.swap->GetCurrentBackBufferIndex()];
						gContext.Defer(rt.allocation);
						gContext.Defer(rt.resource);
						view.frameGraph.ExecuteAsync(sf, gContext, view, rt.resource);
						windows.insert(view.window);
					}			
					gContext.EndScene([windows]()
					{
						for (Window* window : windows)
							window->vp.Present(1);
					});
				}
				catch (const EngineException& e)
				{
					MessageBoxA(nullptr, e.what(), "Engine Exception", MB_OK | MB_ICONERROR);
					return;
				}				
		}).name("RenderJob");
		
		tf::Task updateRenderItemsJob = taskflow.for_each(gScenes.begin(), gScenes.end(), [](Scene& scene)
		{
			scene.FinalizeLogicfFrame();
		}).name("Update Render Items Job");

		dispatchJob.precede(renderJob);
		///logicJob.precede(updateRenderItemsJob);
		renderJob.precede(updateRenderItemsJob);

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

	Scene& CreateScene()
	{
		return gScenes.emplace_back(Scene());
	}

	RenderView& CreateRenderView(Window* pWindow, Scene* pScene, IRenderer* pRenderer)
	{
		RenderView& rv = gRenderViews.emplace_back(RenderView
		{
			.window = pWindow,
			.renderer = pRenderer,
			.scene = pScene,
			.bMarkedForRebuild = false,
		});

		RenderTextureHandle renderOutput = pRenderer->Build(rv.frameGraph);
		rv.frameGraph.AddPass<SubmitToSwapchainPass>(renderOutput, pWindow);	
		rv.frameGraph.Compile(gContext);
		return rv;
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

		i32 argc;
		wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		if (argc > 1)
		{
			for (u32 i = 1; i < argc; ++i)
			{
				std::wstring arg = argv[i];
				if (arg == L"-gfxdebug")
					gDebugGraphics = true;
			}
		}

		if (gDebugGraphics)
		{
			CComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();

				// Enable GPU based validation
				CComPtr<ID3D12Debug1> debugController1;
				if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
				{
					debugController1->SetEnableGPUBasedValidation(true);
				}
			}
		}

		InitializeGFX();
		InitializeGlobalDescriptorHeaps();
		InitializeGlobalCopyQueue();
		fx::InitializeFX(gDevice);

		gContext.Create(TINY_DEFAULT_BUFFERING);
	}

	void Shutdown()
	{
		gRenderViews.clear();
		gScenes.clear();
		gContext.Destroy();
		fx::ShutdonwnFX();
		ShutdownGlobalCopyQueue();
		ShutdownGlobalDescriptorHeaps();
		ShutdownGFX();

		UnregisterClass(TINY_WINDOW_CLASS, GetModuleHandle("Tiny"));
	}
}