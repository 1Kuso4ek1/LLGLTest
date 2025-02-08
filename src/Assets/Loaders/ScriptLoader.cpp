#include <ScriptLoader.hpp>

namespace dev
{

AssetPtr ScriptLoader::Load(const std::filesystem::path& path)
{
    auto asset = std::make_shared<ScriptAsset>(path);
    asset->loaded = true;

    return asset;
}

}
