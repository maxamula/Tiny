#pragma once
#include <shared_mutex>
#include <deque>
#include <variant>

#include "graphics/viewport.h"
#include "graphics/d3dcontext.h"
#include "graphics/framegraph.h"
#include "graphics/renderer.h"
#include "scene.h"
#include "camera.h"
#include "misc/signals.hpp"
#include "misc/dispatcher.h"
#include "meta.h"


namespace tiny
{
	class Window;
	using WindowProc = std::function<LRESULT(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)>;

	struct TINYFX_API WindowDesc
	{
		u16 posX, posY, width, height;
		WindowProc proc;
	};

	class TINYFX_API Window
	{	
	public:
		Window(const WindowDesc& desc);
		DISABLE_COPY(Window);
		DISABLE_MOVE(Window);
		~Window();

		signal<void()> Destroyed;
		signal<void(u16, u16)> Resized;
		signal<void(u16)> KeyDown;
		signal<void(u16)> KeyUp;
		signal<void(u16, u16)> MouseMove;
		signal<void(u16, u16)> MouseDown;
		signal<void(u16, u16)> MouseUp;
	_internal:
		HWND hWnd;
		u16 width, height;
		WindowProc proc;
		std::atomic_bool bNeedResize;
		D3DViewport vp;
	};

	using RenderViewHandle = void*;

	TINYFX_API void LoadScene(const std::string& path);
	TINYFX_API void RunEngine();
	TINYFX_API void CreateRenderView(Window* pWindow, IRenderer* renderer);
	TINYFX_API void RebuildRenderView(RenderViewHandle handle, IRenderer* renderer);

	TINYFX_API void Initialize();
	TINYFX_API void Shutdown();	
}