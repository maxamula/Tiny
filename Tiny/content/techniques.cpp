#include "techniques.h"
#include "assets.h"

namespace tiny
{
	void CreateUnlitTechnique()
	{
		fx::MeshTechnique technique;
		auto opaqueMat = std::make_shared<UnlitMaterialInstance>();
		technique.passes[fx::MeshTechnique::RenderPassId_Opaque] = std::static_pointer_cast<fx::IMeshMaterialInstance>(opaqueMat);

		
	}
}