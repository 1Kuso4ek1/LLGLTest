#pragma once
#include <AssetLoader.hpp>
#include <Multithreading.hpp>

#include <LLGL/Log.h>
#include <LLGL/Texture.h>

#include <filesystem>
#include <typeindex>

namespace dev
{

class AssetManager : public Singleton<AssetManager>
{
public:
    using AssetStorage = std::unordered_map<std::filesystem::path, std::pair<std::type_index, AssetPtr>>;

    template<class T>
    std::shared_ptr<T> Load(const std::filesystem::path& path, bool relativeToAssetsDir = false)
    {
        auto assetPath = path;

        if(relativeToAssetsDir)
        {
            auto relativeAssetPath = assetsRelativePaths.find(std::type_index(typeid(T)));

            if(relativeAssetPath != assetsRelativePaths.end())
                assetPath = assetsDirectory / relativeAssetPath->second / path;
            else
                assetPath = assetsDirectory / path;
        }

        auto it = assets.find(assetPath);

        if(it != assets.end())
        {
            if(typeid(T) == it->second.first)
                return std::static_pointer_cast<T>(it->second.second);
        }

        auto loader = GetAssetLoader<T>();

        if(!loader)
            return nullptr;

        auto asset = loader->Load(assetPath);

        if(!asset)
            return nullptr;

        asset->path = assetPath;

        assets.emplace(assetPath, std::pair(std::type_index(typeid(T)), asset));

        return std::static_pointer_cast<T>(asset);
    }

    template<class T>
    auto GetAssetPath(const std::filesystem::path& path, bool relativeToAssetsDir) const
    {
        auto assetPath = path;

        if(relativeToAssetsDir)
        {
            auto relativeAssetPath = assetsRelativePaths.find(std::type_index(typeid(T)));

            if(relativeAssetPath != assetsRelativePaths.end())
                assetPath = assetsDirectory / relativeAssetPath->second / path;
            else
                assetPath = assetsDirectory / path;
        }

        return assetPath;
    }

    std::filesystem::path GetAssetsDirectory() const
    {
        return assetsDirectory;
    }

    AssetStorage& GetAssets()
    {
        return assets;
    }

    template<class T>
    void Write(std::shared_ptr<T> asset, const std::filesystem::path& path, bool relativeToAssetsDir = false)
    {
        auto assetPath = GetAssetPath<T>(path, relativeToAssetsDir);

        assets.emplace(assetPath, std::pair(std::type_index(typeid(T)), asset));

        auto loader = GetAssetLoader<T>();

        if(!loader)
            return;

        loader->Write(asset, assetPath);

        asset->path = assetPath;
    }

    template<class T>
    void Write(std::shared_ptr<T> asset)
    {
        Write(asset, asset->path);
    }

    void SetAssetsDirectory(const std::filesystem::path& path)
    {
        assetsDirectory = path;
    }

    template<class T>
    void Unload(const std::filesystem::path& path)
    {
        assets.erase(path);
    }

    template<class AssetType, class LoaderType>
    void AddLoader(std::filesystem::path relativePath = "")
    {
        loaders[typeid(AssetType)] = &LoaderType::Get();
        assetsRelativePaths[typeid(AssetType)] = relativePath;
    }

private:
    template<class T>
    AssetLoader* GetAssetLoader()
    {
        auto loader = loaders[std::type_index(typeid(T))];

        if(!loader)
        {
            LLGL::Log::Errorf(
                LLGL::Log::ColorFlags::StdError,
                "No loader found for asset type %s\n", typeid(T).name()
            );

            return nullptr;
        }

        return loader;
    }

private:
    std::filesystem::path assetsDirectory = "assets";

    AssetStorage assets;
    
    std::unordered_map<std::type_index, std::filesystem::path> assetsRelativePaths;
    std::unordered_map<std::type_index, AssetLoader*> loaders;
};

}
