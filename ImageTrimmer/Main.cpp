#include <iostream>
#include <DirectXTex.h>
#include <filesystem>

#include <magic_enum\magic_enum.hpp>

int TrimImage(const std::filesystem::path& filePath)
{
	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

	DirectX::TexMetadata tTextureInfo = { 0 };
	DirectX::ScratchImage tImage;

	std::wstring wFilePath = filePath.wstring();
	std::string extension = filePath.extension().string();

	hr = E_FAIL;
	if (".tga" == extension)
		hr = DirectX::LoadFromTGAFile(wFilePath.data(), &tTextureInfo, tImage);
	else if (".dds" == extension)
		hr = DirectX::LoadFromDDSFile(wFilePath.data(), DirectX::DDS_FLAGS_NONE, &tTextureInfo, tImage);
	else if (".png" == extension || ".bmp" == extension ||
		".jpg" == extension || ".jpeg" == extension ||
		".sheet" == extension)
		hr = DirectX::LoadFromWICFile(wFilePath.data(), DirectX::WIC_FLAGS_NONE, &tTextureInfo, tImage);
	else
		return 0;

	if (FAILED(hr))
	{
		std::cout << "Failed to load Image!" << std::endl;
		return 0;
	}

	std::string_view dxgiFormat = magic_enum::enum_name(tTextureInfo.format);

	std::cout << "File Path : " << filePath << std::endl;
	std::cout << "Format : " << dxgiFormat << std::endl;

	//std::filesystem::path newPath = filePath.parent_path();
	//newPath += filePath.filename();
	//newPath += "_trim";
	//newPath += filePath.extension();

	// find trim point (pivot)
	uint8_t* pTextures = tImage.GetPixels();

	size_t left = INT_MAX;
	size_t top = INT_MAX;
	size_t right = 0;
	size_t bottom = 0;

	for (size_t x = 0; x < tTextureInfo.width; ++x)
	{
		for (size_t y = 0; y < tTextureInfo.height; ++y)
		{
			size_t index = (x + y * tTextureInfo.width) * 4;

			uint8_t r = pTextures[index];
			uint8_t g = pTextures[index + 1];
			uint8_t b = pTextures[index + 2];
			uint8_t a = pTextures[index + 3];

			if (a != 0)
			{
				left = min(left, x);
				top = min(top, y);
				right = max(right, x);
				bottom = max(bottom, y);
			}
		}
	}

	// start trim
	size_t trimLength = (right - left) + 1;
	size_t trimHeight = (bottom - top) + 1;

	uint8_t* pTrimTexture = new uint8_t[trimLength * trimHeight * 4];
	memset(pTrimTexture, 0, trimLength * trimHeight * 4);

	for (size_t x = left; x <= right; ++x)
	{
		for (size_t y = top; y <= bottom; ++y)
		{
			size_t originIndex = (x + y * tTextureInfo.width) * 4;
			// y is 0
			size_t trimIndex = ((x - left) + (y - top) * trimLength) * 4;

			pTrimTexture[trimIndex] = pTextures[originIndex];
			pTrimTexture[trimIndex + 1] = pTextures[originIndex + 1];
			pTrimTexture[trimIndex + 2] = pTextures[originIndex + 2];
			pTrimTexture[trimIndex + 3] = pTextures[originIndex + 3];

		}
	}

	DirectX::Image image;
	image.pixels = pTrimTexture;
	image.width = trimLength;
	image.height = trimHeight;
	image.format = tTextureInfo.format;
	image.rowPitch = trimLength * 4;
	image.slicePitch = trimLength * trimHeight * 4;

	hr = DirectX::SaveToWICFile(image,
		DirectX::WIC_FLAGS_FORCE_RGB,
		DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
		wFilePath.data());

	if (FAILED(hr))
	{
		std::cout << "Failed to trim Image!" << std::endl;
	}
	else
	{
		std::cout << "Success, Trimmed Size " << trimLength << " " << trimHeight << std::endl;
	}
	delete[] pTrimTexture;

	return 0;
}

void RecursiveTrim(const std::filesystem::path& directoryPath)
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath))
	{
		if (entry.is_directory())
		{
			RecursiveTrim(entry.path());
			continue;
		}
		TrimImage(entry.path());
	}
}

int main(int argc, char** pArgs)
{
	std::filesystem::path directoryPath = "./";

	if (argc > 01)
	{
		directoryPath = pArgs[1];
	}

	RecursiveTrim(directoryPath);

	return 0;
}