#pragma once
#include <shared_mutex>
#include <deque>
#include <variant>

#include "graphics/viewport.h"
#include "graphics/d3dcontext.h"
#include "graphics/framegraph.h"
#include "graphics/renderer.h"
#include "scene.h"
#include "misc/dispatcher.h"
#include "meta.h"

struct ImGuiContext;

namespace tiny
{
	class Window;
	using WindowProc = std::function<LRESULT(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)>;

	struct TINYFX_API WindowDesc
	{
		u16 posX, posY, width, height;
		WindowProc proc;
		HWND hParent;
		bool bFullscreen;
		std::string title;
	};

	struct KeyEvent
	{
		u16	key;
		bool pressed;
		bool repeat;
	};

	struct MouseMoveEvent
	{
		f32 dx, dy;
	};

	struct MouseWheelEvent 
	{
		f32 delta;
	};

	struct MouseButtonEvent
	{
		enum MouseButton : u8
		{
			MouseButton_Left,
			MouseButton_Right,
			MouseButton_Middle
		};
		MouseButton button;
		bool pressed;
	};

	struct LockedBus;

	class TINYFX_API Window
	{	
	public:
		Window(const WindowDesc& desc);
		DISABLE_COPY(Window);
		DISABLE_MOVE(Window);
		~Window();
		
		void Show();
		void Hide();
		void SetTitle(const std::string& title);
		void SetFullscreen(bool fullscreen);
		void SetRect(u16 x, u16 y, u16 width, u16 height);

		entt::dispatcher& InputBus();
	_internal:
		HWND hWnd;
		u16 width, height;
		WindowProc proc;
		std::atomic_bool bNeedResize;
		D3DViewport vp;
		ImGuiContext* mImContext;
		DescriptorHandle mImguiSrv;
		LockedBus& mDispatcher;
	};

	struct TINYFX_API RenderView
	{
		inline void Rebuild() { bMarkedForRebuild = true; }
	_internal:
		Window* window;
		IRenderer* renderer;
		FrameGraph frameGraph;
		Scene* scene;
		bool bMarkedForRebuild;
	};

	TINYFX_API void RunEngine();
	TINYFX_API Scene& CreateScene();
	TINYFX_API RenderView& CreateRenderView(Window* pWindow, Scene* pScene, IRenderer* renderer);
	TINYFX_API void Initialize();
	TINYFX_API void Shutdown();	
}