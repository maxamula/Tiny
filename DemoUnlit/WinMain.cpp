#include <Windows.h>
#define TINY_REVEAL_INTERNALS
#include <engine/application.h>
#include <content/assets.h>
#include <filesystem>
using namespace DirectX;

namespace fs = std::filesystem;

tiny::Window* gpWindow;

class CamScript : public tiny::IScript
{
public:
    CamScript(tiny::Scene& scene)
        : mScene(scene)
    {
        auto& bus = gpWindow->InputBus();
        bus.sink<tiny::KeyEvent>().connect<&CamScript::OnKeyEvent>(this);
        bus.sink<tiny::MouseMoveEvent>().connect<&CamScript::OnMouseMove>(this);
        bus.sink<tiny::MouseButtonEvent>().connect<&CamScript::OnMouseButtonEvent>(this);
        bus.sink<tiny::MouseWheelEvent>().connect<&CamScript::OnMouseWheelEvent>(this);
    }

    void OnUpdate(entt::entity entity, float deltaTime) override
    {
        using namespace DirectX;
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);
        XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotation);
        XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotation);
        XMVECTOR up = XMVector3Cross(right, forward);

        XMVECTOR move = XMVectorZero();
        if (mMoveForward)  move += forward;
        if (mMoveBackward) move -= forward;
        if (mMoveRight)    move += right;
        if (mMoveLeft)     move -= right;
        if (mMoveUp)       move += up;
        if (mMoveDown)     move -= up;

        if (XMVector3LengthSq(move).m128_f32[0] > 0.0f)
        {
            move = XMVector3Normalize(move) * mSpeed * deltaTime;
        }

        mScene.Modify<tiny::ComponentCamera>(entity, [&](tiny::ComponentCamera& camera)
        {
                camera.position = XMVectorAdd(camera.position, move);
                camera.lookDir = XMVector3Normalize(forward);
        });
    }

	void OnFixedUpdate(entt::entity entity) override
	{
	}

private:
    tiny::Scene& mScene;
    float mSpeed = 5.0f;
    float mYaw = 0.0f, mPitch = 0.0f;
    float mMouseSensitivity = 0.002f;

    bool mRightMouseDown = false;
    bool mMoveForward = false;
    bool mMoveBackward = false;
    bool mMoveLeft = false;
    bool mMoveRight = false;
    bool mMoveUp = false;
    bool mMoveDown = false;

    void OnKeyEvent(tiny::KeyEvent& event)
    {
        bool down = event.pressed;
        switch (event.key)
        {
        case 'W': mMoveForward = down;  break;
        case 'S': mMoveBackward = down; break;
        case 'A': mMoveLeft = down;     break;
        case 'D': mMoveRight = down;    break;
        case VK_SPACE:     mMoveUp = down;   break;
        case VK_CONTROL:   mMoveDown = down; break;
        case VK_SHIFT:
            if (down && !event.repeat) mSpeed *= 2.0f;
            else if (!down)            mSpeed *= 0.5f;
            break;
        }
    }

    void OnMouseButtonEvent(tiny::MouseButtonEvent& event)
    {
        if (event.button == tiny::MouseButtonEvent::MouseButton_Right)
        {
            mRightMouseDown = event.pressed;
            ShowCursor(!mRightMouseDown);
        }
    }

    void OnMouseMove(tiny::MouseMoveEvent& event)
    {
        if (!mRightMouseDown)
            return;
        mYaw += event.dx * mMouseSensitivity;
        mPitch += event.dy * mMouseSensitivity;
        const float limit = XM_PI / 2.0f - 0.01f;
        mPitch = std::clamp(mPitch, -limit, limit);
    }

    void OnMouseWheelEvent(tiny::MouseWheelEvent& event)
    {
        mSpeed *= (1.0f + event.delta * 0.1f);
        mSpeed = std::max(mSpeed, 0.1f);
    }
};

class MoveScript : public tiny::IScript
{
public:
    MoveScript(tiny::Scene& scene, entt::entity entity)
		: mScene(scene), entity(entity)
    { 
		gpWindow->InputBus().sink<tiny::KeyEvent>().connect<&MoveScript::OnKeyEvent>(this);
    }

    void OnUpdate(entt::entity entity, float deltaTime) override
    {

    }

    void OnFixedUpdate(entt::entity entity) override
    {
    }

private:
	void OnKeyEvent(tiny::KeyEvent& event)
	{
		if (event.key == VK_UP && event.pressed && !event.repeat)
		{
			mScene.Modify<tiny::ComponentTransform>(entity, [&](tiny::ComponentTransform& tf)
			{
					tf.position += DirectX::XMVectorSet(0.0f, 0.0f, 0.1f, 0.0f);
			});
		}
		if (event.key == VK_DOWN && event.pressed && !event.repeat)
		{
			mScene.Modify<tiny::ComponentTransform>(entity, [&](tiny::ComponentTransform& tf)
				{
					tf.position += DirectX::XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f);
				});
		}
		if (event.key == VK_LEFT && event.pressed && !event.repeat)
		{
			mScene.Modify<tiny::ComponentTransform>(entity, [&](tiny::ComponentTransform& tf)
				{
					tf.position += DirectX::XMVectorSet(-0.1f, 0.0f, 0.0f, 0.0f);
				});
		}
		if (event.key == VK_RIGHT && event.pressed && !event.repeat)
		{
			mScene.Modify<tiny::ComponentTransform>(entity, [&](tiny::ComponentTransform& tf)
				{
					tf.position += DirectX::XMVectorSet(0.1f, 0.0f, 0.0f, 0.0f);
				});
		}
	}

    tiny::Scene& mScene;
	entt::entity entity;
};

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	tiny::Initialize();
	{
		tiny::WindowDesc desc
		{
			.posX = 100,
			.posY = 100,
			.width = 1280,
			.height = 720,
			.proc = [](tiny::Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				if (message == WM_CLOSE)
					PostQuitMessage(0);
				return DefWindowProc(hWnd, message, wParam, lParam);
			},
			.title = "Tiny Engine Demo"
		};
		tiny::Window window(desc);
		gpWindow = &window;
		tiny::TinyForwardRenderer renderer;
		renderer.SetResolution(1280, 720);
		tiny::Scene& scene = tiny::CreateScene();
		tiny::RenderView& renderView = tiny::CreateRenderView(&window, &scene, &renderer);

		entt::entity chessboardEntity = scene.CreateObject("Chessboard");
		scene.Emplace<tiny::ComponentTransform>(chessboardEntity, DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
		scene.Emplace<tiny::ComponentGeometry>(chessboardEntity, tiny::AssetCreateMesh(tiny::AssetLoadMesh("Sponza\\Chessboard.asset")));
		scene.Emplace<tiny::ComponentMaterial>(chessboardEntity, tiny::AssetCreateTechnique(tiny::AssetLoadTechnique("Sponza\\Chessboard.technique")));

		entt::entity kingEntty = scene.CreateObject("King");
		scene.Emplace<tiny::ComponentTransform>(kingEntty, DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
		scene.Emplace<tiny::ComponentGeometry>(kingEntty, tiny::AssetCreateMesh(tiny::AssetLoadMesh("Sponza\\King_Shared.asset")));
		scene.Emplace<tiny::ComponentMaterial>(kingEntty, tiny::AssetCreateTechnique(tiny::AssetLoadTechnique("Sponza\\King_Shared.technique")));
		scene.Emplace<tiny::ComponentScript>(kingEntty, std::make_unique<MoveScript>(scene, kingEntty));

		entt::entity cameraEntty = scene.CreateObject("camera");
		scene.Emplace<tiny::ComponentCamera>(cameraEntty, DirectX::XMVectorSet(0.0f, 0.3f, -0.2f, 1.0f), DirectX::XMVectorSet(0.0f, -0.3f, .2f, 0.0f), 60.0f, 0.1f, 100.0f);
		scene.Emplace<tiny::ComponentTransform>(cameraEntty, DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f));
		scene.Emplace<tiny::ComponentScript>(cameraEntty, std::make_unique<CamScript>(scene));

		window.Show();
		tiny::RunEngine();
	}
	tiny::Shutdown();
	return 0;
}