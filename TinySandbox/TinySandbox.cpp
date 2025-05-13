#pragma region includes
#include <Tiny/fx/registry.h>
#include <Tiny/fx/shaderfx.h>
#include <Tiny/content/stdmat.h>
#include <Tiny/content/assets.h>
#include <Tiny/engine/application.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <string>
#include <vector>
#include <variant>
#include <any>

#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/archives/json.hpp"

#include "graphics/pass.h"
#include "graphics/renderer.h"

#include <fstream>
#include "Tiny/common.h"

#include "graphics/framegraph.h"
#pragma endregion
#include <iostream>

int main()
{
	try
	{

	}
	catch (const tiny::DxException& e)
	{
		MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
	}
}