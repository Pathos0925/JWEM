#include "com_ptr.hpp"
#include "d3d11/d3d11.hpp"
#include "DirectXTex/DirectXTex.h"

#include "log.hpp"


#include <stddef.h>
#include <stdint.h>
#include "d3d11/d3d11_runtime.hpp"

#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <iostream>
//#include <filesystem>
#include "filesystem.hpp"
#include "directory_watcher.hpp"

#pragma comment(lib, "windowsapp")

#include <Windows.Foundation.h>
#include <wrl\wrappers\corewrappers.h>
#include <wrl\client.h>
#include <stdio.h>
#include <cstdlib>
#include <thread>

#include <fstream>

//#include "bit7z/include/bit7z.hpp"
//#include "bit7z/include/bit7zlibrary.hpp"


struct thread_data {
	int  thread_id;
	char *message;
};

struct texture_item
{
	std::string type;
	int dataStart;
	int dataLength;
};

/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define POLY 0x82f63b78
#define JWEM_VERSION_STRING_FILE "0.1"


//namespace fs = std::filesystem;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

class Singleton {
	static Singleton *instance;

	int data;
	// Private constructor so that no objects can be created.
	Singleton() {
		data = 0;
		GetDebugTexList() = {};
		savingImage = false;
		DebugInitialCrcList = {};
		//init();
	}

public:

	void init(std::string startFolder)
	{
		LOG(DEBUG) << "JWEM init";
#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
		Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
		if (FAILED(initialize))
		{
			LOG(ERROR) << "Failed to initialize Windows Runtime and COM.";
		}
		else
		{
			LOG(DEBUG) << "initialized Windows Runtime and COM";
		}
#else
		HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
		if (FAILED(initialize))
		{
			LOG(ERROR) << "Failed to initialize Windows Runtime and COM.";
		}
		else
		{
			LOG(DEBUG) << "initialized Windows Runtime and COM";
		}
#endif

		DLLDirectory = startFolder;
		saveTextureDirectory = startFolder + "\\SavedTextures";
		skinsDirectory = startFolder + "\\Skins";

		//LOG(INFO) << "saveTextureDirectory: " << saveTextureDirectory;
		//LOG(INFO) << "skinsDirectory: " << skinsDirectory;

		ReloadDiscoveredTextures();
	}

	std::mutex mtx;

	/*
	std::map<uint32_t, std::string> crcTextureName = {


	};
	*/
	std::vector< uint32_t> crcListCrc;
	std::vector< std::string> crcListName;



	std::map<std::string, std::string> idToNameList = {
	{ "ANKY-0", "Ankylosaurus Basic" },
	{ "ARCH-0", "Archaeornithomimus Basic" },
	};

	std::map<std::string, uint32_t> discoveredSkins = {

	};



	//make this a map?
	std::vector<com_ptr<ID3D11Texture2D>> DinoTexList;
	std::vector<uint32_t> DinoCrcList;

	std::vector<com_ptr<ID3D11Texture2D>> DebugTexList;
	std::vector<uint32_t> DebugInitialCrcList;
	//

	//JWEMS filename
	std::vector< std::vector<texture_item>> JWEMSTextureDataInfo;
	std::vector<std::string> JWEMSTextureFiles;
	std::vector<std::string> JWEMSTextureSpecies;


	//data, size
	std::map<unsigned char*, int> crcQueue = {

	};
	std::vector<com_ptr<ID3D11Texture2D>> crcQueuePointer;

	bool multiThreadCrc = false;

	//gui vars
	bool Enabled = true;

	std::string saveTextureDirectory;
	std::string skinsDirectory;
	std::string DLLDirectory;
	bool CrcAsName = true;
	bool MonitorTextures = false;
	UINT RecordTextureCutoffSize = 1024;
	std::string LoadTextureName = "test.dds";

	DirectX::ScratchImage LoadedImage;
	bool ImageIsLoaded = false;
	int LoadedImageWidth = 512;
	int LoadedImageHeight = 512;

	ULONG LoadedTextureRef = 0;
	bool DisableReshade = false;
	//--

	long long lastPresentDuration = 0;

	ID3D11DeviceContext * singleContext;
	ID3D11Device * originalDevice;


	bool savingImage = false;
	const DWORD texFlags = DirectX::DDS_FLAGS_NONE;


	static Singleton *getInstance() {
		if (!instance)
			instance = new Singleton;
		return instance;
	}

	int getData() {
		return this->data;
	}

	void setData(int data) {
		this->data = data;
	}

	template<typename K, typename V>
	bool findByValue(std::vector<K> & vec, std::map<K, V> mapOfElemen, V value)
	{
		bool bResult = false;
		auto it = mapOfElemen.begin();
		// Iterate through the map
		while (it != mapOfElemen.end())
		{
			// Check if value of this entry matches with given value
			if (it->second == value)
			{
				// Yes found
				bResult = true;
				// Push the key in given map
				vec.push_back(it->first);
			}
			// Go to next entry in map
			it++;
		}
		return bResult;
	}

	char *convert(const std::string & s)
	{
		char *pc = new char[s.size() + 1];
		std::strcpy(pc, s.c_str());
		return pc;
	}

	std::string GetFileExtension(const std::string& FileName)
	{
		if (FileName.find_last_of(".") != std::string::npos)
			return FileName.substr(FileName.find_last_of(".") + 1);
		return "";
	}
	std::string GetFileDirectory(const std::string& FileName)
	{
		if (FileName.find_last_of("\\") != std::string::npos)
			return FileName.substr(0, FileName.find_last_of("\\") + 1);
		return "";
	}
	std::string GetFilenameFromPath(std::string filename, bool removeExtension)
	{
		// Remove directory if present.
		// Do this before extension removal incase directory has a period character.
		const size_t last_slash_idx = filename.find_last_of("\\/");
		if (std::string::npos != last_slash_idx)
		{
			filename.erase(0, last_slash_idx + 1);
		}

		// Remove extension if present.
		if (removeExtension)
		{
			const size_t period_idx = filename.rfind('.');
			if (std::string::npos != period_idx)
			{
				filename.erase(period_idx);
			}
		}
		return filename;
	}

	void ReloadDiscoveredTextures()
	{
		std::ifstream crcFile(DLLDirectory + "\\FULLCRC_LIST.txt");
		if (crcFile.good())
		{
			int idCount = 0;

			//crcTextureName.clear();
			crcListCrc.clear();
			crcListName.clear();

			for (std::string line; getline(crcFile, line); )
			{
				try
				{
					if (line.length() <= 2)
						continue;
					if (line.substr(0, 1) == "/")
						continue;

					auto found = line.find('=');
					uint32_t crc = 0;
					if (found != std::string::npos)
					{
						std::string crcString = line.substr(0, found);
						crc = std::stoul(crcString);

						std::string id = line.substr(found + 1, line.length() - found);

						//crcTextureName.insert(std::pair<uint32_t, std::string>(crc, id));
						crcListName.push_back(id);
						crcListCrc.push_back(crc);
						idCount++;
					}
				}
				catch (...)
				{
					//LOG(ERROR) << "Error reading crc list!";
					continue;
				}

			}
			LOG(DEBUG) << "Found " << idCount << " crcs";
		}
		else
		{
			LOG(ERROR) << "crc list not found, nothing will work.";
		}

		std::ifstream skinIdFile(DLLDirectory + "\\SKIN_IDS.txt");
		if (skinIdFile.good())
		{
			int idCount = 0;

			idToNameList.clear();
			for (std::string line; getline(skinIdFile, line); )
			{
				try
				{

					if (line.length() <= 2)
						continue;
					if (line.substr(0, 1) == "/")
						continue;

					auto found = line.find('=');

					if (found != std::string::npos)
					{
						std::string skinId = line.substr(0, found);

						std::string skinName = line.substr(found + 1, line.length() - found);

						idToNameList.insert(std::pair<std::string, std::string>(skinId, skinName));
						idCount++;
					}
				}
				catch (...)
				{
					//LOG(ERROR) << "Error reading skinIdFile!";
					continue;
				}

			}
			//LOG(DEBUG) << "Found " << idCount << " skin ids";
		}
		else
		{
			LOG(ERROR) << "skinIdFile not found, nothing will work.";
		}


		JWEMSTextureDataInfo.clear();
		JWEMSTextureFiles.clear();
		JWEMSTextureSpecies.clear();


		//crcTextureName
		discoveredSkins.clear();

		//std::string dir = skinsDirectory + "\\" + x.second + "\\";
		std::string dir = skinsDirectory + "\\";
		//LOG(DEBUG) << "checking: " << dir;

		const std::vector<reshade::filesystem::path> matching_files = reshade::filesystem::list_files(dir, "*.*");

		for (auto & p : matching_files)
		{
			auto extension = GetFileExtension(p.filename().string());
			std::transform(extension.begin(), extension.end(), extension.begin(), ::toupper);

			if (extension != std::string("DDS")
				&& extension != std::string("PNG")
				&& extension != std::string("BMP")
				&& extension != std::string("GIF")
				&& extension != std::string("TIFF")
				&& extension != std::string("JPEG")
				&& extension != std::string("JWEMS"))
				continue;

			std::string filename = GetFilenameFromPath(p.filename_without_extension().string(), false);

			if (extension == "JWEMS")
			{
				//process JWEMS
				ProcessJWEMS(dir + p.filename().string());

			}
			else
			{
				//std::string fullPath = p.filename;
				if (GetFilenameFromPath(p.filename_without_extension().string(), true).length() < 15)
					continue;

				std::string id = filename.substr(filename.length() - 13, 13);

				//std::vector<uint32_t> vec;
				std::vector<std::string>::iterator it = std::find(crcListName.begin(), crcListName.end(), id);
				if (it != crcListName.end())
				{
					int index = std::distance(crcListName.begin(), it);

					std::string combined = dir + p.filename().string();
					discoveredSkins.insert(std::pair<std::string, uint32_t>(combined, crcListCrc[index]));
					//LOG(DEBUG) << "Found skin for " << id << " at: " << combined;
				}
				/*
				if (findByValue(vec, crcTextureName, id))
				{
					std::string combined = dir + p.filename().string();
					discoveredSkins.insert(std::pair<std::string, uint32_t>(combined, vec[0]));
					//LOG(DEBUG) << "Found skin for " << id << " at: " << combined;
				}
				*/
			}

		}



	}

	void ProcessJWEMS(std::string filename)
	{

		LOG(DEBUG) << "==Processing JWEMS==";
		//std::string dlldir = DLLDirectory + "\\7za.dll";
		//std::wstring dirw = std::wstring(dlldir.begin(), dlldir.end());

		//std::wstring filenamew = std::wstring(filename.begin(), filename.end());

		//bit7z::Bit7zLibrary lib(dirw);//dirw
		//bit7z::BitExtractor extractor(lib, bit7z::BitFormat::SevenZip);
		//std::vector<bit7z::byte_t> buffer;

		/*
		LOG(DEBUG) << "-Extracting...";
		try
		{
			extractor.extract(filenamew, buffer);
		}
		catch (...)
		{
			LOG(ERROR) << "-Couldent extract!";
		}
		*/

		std::fstream input(filename, std::ios_base::in | std::ios_base::binary);
		texture_item item;

		std::string line;

		std::getline(input, line);
		JWEMSTextureSpecies.push_back(line);
		std::getline(input, line);
		int texCount = std::stoi(line);
		std::vector<texture_item> newList;
		for (int i = 0; i < texCount; i++)
		{
			std::getline(input, line);
			std::string texType = line;
			std::getline(input, line);
			int dataLength = std::stoi(line);
			texture_item newItem;
			newItem.type = texType;
			newItem.dataLength = dataLength;
			newItem.dataStart = input.tellg();
			input.seekg(newItem.dataStart + newItem.dataLength);
			newList.push_back(newItem);
		}

		JWEMSTextureDataInfo.push_back(newList);
		//itemList.pars
		JWEMSTextureFiles.push_back(filename);
		LOG(DEBUG) << "-complete!";
	}

	std::vector<std::string> getSkinsForCrc(uint32_t crc, std::vector<std::string> * fileNames)
	{

		std::vector<std::string> files;
		//std::vector<std::string> fileNames;
		for (auto const& value : discoveredSkins)
		{
			if (value.second == crc)
			{
				files.push_back(value.first);
				std::string filename = GetFilenameFromPath(value.first, true);
				filename = filename.substr(0, filename.length() - 14);
				fileNames->push_back(filename);
			}
		}
		//std::string Id = crcTextureName[crc].substr(0, 6);
		std::string Id;

		std::vector<uint32_t>::iterator it = std::find(crcListCrc.begin(), crcListCrc.end(), crc);
		if (it != crcListCrc.end())
		{
			int index = std::distance(crcListCrc.begin(), it);
			Id = crcListName[index];
		}

		std::string SpeciesName = idToNameList[Id];
		SpeciesName = SpeciesName.substr(0, SpeciesName.find(" "));
		int index = 0;
		for (auto const& value : JWEMSTextureSpecies)
		{
			if (value == SpeciesName)
			{
				files.push_back(JWEMSTextureFiles[index]);
				std::string filename = GetFilenameFromPath(JWEMSTextureFiles[index], true);
				fileNames->push_back(filename);
			}
			index++;
		}

		return files;
	}

	bool update_texture(int width, int height, DXGI_FORMAT format, const uint8_t *data, com_ptr<ID3D11Texture2D> tex, int index = 0)
	{

		LOG(DEBUG) << "====update_texture====";

		if (data == nullptr)
		{
			LOG(ERROR) << "data is nullptr!";
			return false;
		}

		/*
		if (format == 70)
		{
			format = reshade::texture_format::rg8;
		}
		*/



		switch (format)
		{
			/*
			case reshade::texture_format::r8:
			{
				std::vector<uint8_t> data2(desc->Width * desc->Height);
				for (size_t i = 0, k = 0; i < desc->Width * desc->Height * 4; i += 4, k++)
					data2[k] = data[i];
				singleContext->UpdateSubresource(tex.get(), index, nullptr, data2.data(), desc->Width, desc->Width * desc->Height);
				break;
			}
			*/
		case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
		{
			std::vector<uint8_t> data2(width * height * 2);
			for (size_t i = 0, k = 0; i < width * height * 4; i += 4, k += 2)
				data2[k] = data[i],
				data2[k + 1] = data[i + 1];
			singleContext->UpdateSubresource(tex.get(), index, nullptr, data2.data(), height * 2, width * height * 2);
			break;
		}
		default:
		{
			singleContext->UpdateSubresource(tex.get(), index, nullptr, data, width * 4, width * height * 4);
			break;
		}
		}

		return true;
	}


	bool addTex(com_ptr<ID3D11Texture2D> tex, uint32_t initialCrc)
	{
		if (initialCrc == 0)
			return false;

		//LOG(DEBUG) << "==addTex== " << initialCrc;

		if (tex == nullptr)
		{
			LOG(ERROR) << "tex == nullptr" << initialCrc;
		}


		mtx.lock();

		std::vector<uint32_t>::iterator it = std::find(crcListCrc.begin(), crcListCrc.end(), initialCrc);
		if (it != crcListCrc.end())
		{
			DinoTexList.push_back(tex);
			DinoCrcList.push_back(initialCrc);
		}
		/*
		std::map<uint32_t, std::string>::iterator it = crcTextureName.find(initialCrc);
		if (it != crcTextureName.end())
		{
			DinoTexList.push_back(tex);
			DinoCrcList.push_back(initialCrc);
		}
		*/
		if (MonitorTextures)
		{
			DebugTexList.push_back(tex);
			DebugInitialCrcList.push_back(initialCrc);
		}
		mtx.unlock();

		return false;
	}
	std::vector<com_ptr<ID3D11Texture2D>> GetDinoTexList()
	{
		mtx.lock();
		std::vector<com_ptr<ID3D11Texture2D>> newList = DinoTexList;
		mtx.unlock();
		return newList;
	}
	std::vector<uint32_t> GetDinoCrcList()
	{
		mtx.lock();
		std::vector<uint32_t> newList = DinoCrcList;
		mtx.unlock();
		return newList;
	}

	std::vector<com_ptr<ID3D11Texture2D>> GetDebugTexList()
	{
		mtx.lock();
		std::vector<com_ptr<ID3D11Texture2D>> newList = DebugTexList;
		mtx.unlock();
		return newList;
	}
	std::vector<uint32_t> GetDebugInitialCrcList()
	{
		mtx.lock();
		std::vector<uint32_t> newList = DebugInitialCrcList;
		mtx.unlock();
		return newList;
	}


	void UpdateDebugTexture(int index)
	{
		LOG(DEBUG) << "==UpdateDebugTexture==" << index;

		LOG(DEBUG) << "-LoadTextureFromFile";
		std::string texName = LoadTextureName;
		DirectX::ScratchImage loadedImage = LoadTextureFromFile(texName, false);

		DirectX::ScratchImage * finalImage = &loadedImage;
		auto texToUpdate = GetDebugTexList()[index].get();

		ProcessAndLoadImageIntoPoiner(finalImage, texToUpdate);

	}
	void Start_UpdateDebugTexture(int index)
	{
		static int staticIndex = index;
		auto Thread = std::thread([this] { this->UpdateDebugTexture(staticIndex); });
		Thread.detach();
	}
	void UpdateDinoTexture(int index, std::string filename)
	{

		LOG(DEBUG) << "==UpdateDinoTexture:" << index;

		std::string extension = GetFileExtension(filename);
		if (extension == "JWEMS")
		{
			//get JWEMS by full filename
			ptrdiff_t pos = find(JWEMSTextureFiles.begin(), JWEMSTextureFiles.end(), filename) - JWEMSTextureFiles.begin();

			auto datainfolist = JWEMSTextureDataInfo[pos];
			//with the datainfo from that, for each texture, load it
			for (int i = 0; i < datainfolist.size(); i++)
			{
				LOG(INFO) << "-loading " << (i + 1) << " of " << datainfolist.size();

				auto info = datainfolist[i];
				std::string type = "A";

				if (info.type == "NormalMap")
				{
					type = "N";
				}
				else if (info.type == "Packed")
				{
					type = "P";
				}
				std::string speciesSkinName = JWEMSTextureSpecies[pos] + " Basic";
				std::vector<std::string> ids;
				std::string id;
				if (!findByValue(ids, idToNameList, speciesSkinName))
				{
					LOG(ERROR) << " Couldent find id in JWEMS";
				}
				id = ids[0];
				id = id + "-1024-" + type;

				/*
				std::vector<uint32_t> vec;
				if (!findByValue(vec, crcTextureName, id))
				{
					LOG(ERROR) << " Couldent crc for id in JWEMS";
				}
				*/
				uint32_t crcPosInList;
				std::vector<std::string>::iterator it = std::find(crcListName.begin(), crcListName.end(), id);
				if (it != crcListName.end())
				{
					int index = std::distance(crcListName.begin(), it);
					crcPosInList = crcListCrc[index];
				}

				ptrdiff_t cpos = find(DinoCrcList.begin(), DinoCrcList.end(), crcPosInList) - DinoCrcList.begin();
				auto texToUpdate = DinoTexList[cpos];

				char * DDSDataBuffer = new char[info.dataLength];

				std::ifstream ifs(filename, std::ios_base::binary | std::ios_base::ate);
				ifs.seekg(info.dataStart);
				ifs.read(DDSDataBuffer, info.dataLength);

				DirectX::TexMetadata metadata;
				DirectX::ScratchImage loadedimage;
				auto result = DirectX::LoadFromDDSMemory(DDSDataBuffer,
					info.dataLength,
					DirectX::DDS_FLAGS_NONE, &metadata, loadedimage);


				if (FAILED(result))
				{
					LOG(ERROR) << "-LoadFromDDSMemory failed";
					continue;
				}

				ProcessAndLoadImageIntoPoiner(&loadedimage, texToUpdate);

				LOG(INFO) << "-loaded " << (i + 1) << " of " << datainfolist.size();
			}
			return;
		}

		std::vector<std::string> accompaniedSkins = GetAccompaniedSkins(filename);

		for (int i = 0; i < accompaniedSkins.size(); i++)
		{
			std::string individualSkinName = accompaniedSkins[i];
			std::string skinId = GetFilenameFromPath(individualSkinName, true);


			skinId = skinId.substr(skinId.length() - 13, 13);
			//check if we have a crc for this id

			std::vector<std::string>::iterator it = std::find(crcListName.begin(), crcListName.end(), skinId);
			if (it != crcListName.end())
			{
				int index = std::distance(crcListName.begin(), it);
				auto foundCrc = crcListCrc[index];
				if (std::find(DinoCrcList.begin(), DinoCrcList.end(), foundCrc) != DinoCrcList.end())
				{
					LOG(DEBUG) << "-LoadTextureFromFile: " << individualSkinName;
					DirectX::ScratchImage loadedImage = LoadTextureFromFile(individualSkinName, true);


					LOG(DEBUG) << "-GetDinoTexList";
					auto texToUpdate = GetDinoTexList()[index].get();


					ProcessAndLoadImageIntoPoiner(&loadedImage, texToUpdate);
					//Loadimageintoptr

				}
			}
			/*
			std::vector<uint32_t> vec;
			if (findByValue(vec, crcTextureName, skinId))
			{
				if (std::find(DinoCrcList.begin(), DinoCrcList.end(), vec[0]) != DinoCrcList.end())
				{
					LOG(DEBUG) << "-LoadTextureFromFile: " << individualSkinName;
					DirectX::ScratchImage loadedImage = LoadTextureFromFile(individualSkinName, true);


					LOG(DEBUG) << "-GetDinoTexList";
					auto texToUpdate = GetDinoTexList()[index].get();


					ProcessAndLoadImageIntoPoiner(&loadedImage, texToUpdate);
					//Loadimageintoptr

				}
			}
			*/


		}

	}
	void ProcessAndLoadImageIntoPoiner(DirectX::ScratchImage * sloadedImage, com_ptr<ID3D11Texture2D> texToUpdate)
	{
		DirectX::ScratchImage loadedImage = std::move(*sloadedImage);

		D3D11_TEXTURE2D_DESC desc;
		texToUpdate->GetDesc(&desc);

		int mipsToGenerate = 10;
		if (desc.Width == 1024)
		{
			mipsToGenerate = 11;
		}
		else if (desc.Width == 2048)
		{
			mipsToGenerate = 12;
		}

		/*

		*/

		if (loadedImage.GetMetadata().format == desc.Format && loadedImage.GetImageCount() >= mipsToGenerate)
		{
			//dont need to do anything

			//load images
			//return;
		}
		else
		{


			if (loadedImage.GetMetadata().format != desc.Format)
			{
				if (DirectX::IsCompressed(loadedImage.GetMetadata().format))
				{
					//decompress
					LOG(DEBUG) << "-Decompress";
					DirectX::ScratchImage outImage;
					auto hr = Decompress(loadedImage.GetImages(), loadedImage.GetImageCount(),
						loadedImage.GetMetadata(),
						DXGI_FORMAT_UNKNOWN, outImage);
					loadedImage = std::move(outImage);

					if (FAILED(hr))
					{
						LOG(ERROR) << "-Decompress failed";
						return;
					}

				}

				if (loadedImage.GetImageCount() < mipsToGenerate)
				{
					//get mips
					LOG(DEBUG) << "-GenerateMipMaps";
					DirectX::ScratchImage outImage;
					auto hr = GenerateMipMaps(loadedImage.GetImages(), loadedImage.GetImageCount(),
						loadedImage.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, mipsToGenerate, outImage);
					if (FAILED(hr))
					{
						LOG(WARNING) << "-Couldent GenerateMipMaps";
					}
					loadedImage = std::move(outImage);

				}
				if (desc.Format == DXGI_FORMAT_BC1_TYPELESS)
					desc.Format = DXGI_FORMAT_BC1_UNORM;

				//DXGI_FORMAT_BC1_UNORM 
				if (DirectX::IsCompressed(desc.Format))
				{
					//Compression is extremely slow if not using a gpu

					LOG(DEBUG) << "-Compress. this could take a long time...";
					DirectX::ScratchImage outImage;
					auto hr = Compress(originalDevice, loadedImage.GetImages(), loadedImage.GetImageCount(),
						loadedImage.GetMetadata(), desc.Format,
						DirectX::TEX_COMPRESS_FLAGS::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,
						outImage);
					loadedImage = std::move(outImage);
					if (FAILED(hr))
					{
						LOG(ERROR) << "-Recompress failed";
						return;
					}
				}
			}
		}

		for (int i = 0; i < loadedImage.GetImageCount(); i++)
		{
			//LOG(DEBUG) << "-GetImage " << i;
			const DirectX::Image * img = loadedImage.GetImage(i, 0, 0);

			if (img == nullptr)
				break;


			//LOG(DEBUG) << "-get pixels ";
			const uint8_t *pPixels = img->pixels;

			if (pPixels != nullptr)
			{
				LOG(DEBUG) << "-update_texture mip " << i << " of " << loadedImage.GetImageCount();




				if (update_texture(img->width, img->height, img->format, pPixels, texToUpdate, i))
				{
					LOG(DEBUG) << "==update_texture SUCCESS==";
				}
				else
				{
					LOG(DEBUG) << "-update_texture FAILED";
				}
			}
			else
			{
				LOG(ERROR) << "-pixels are null";
			}
		}
		LOG(DEBUG) << "-update complete";

		return;
	}

	void Start_CRCTextureData(unsigned char* data, int size, com_ptr<ID3D11Texture2D> texture)
	{
		static unsigned char* staticData = data;
		static int staticSize = size;
		static com_ptr<ID3D11Texture2D> statictexture = texture;
		auto Thread = std::thread([this]
		{
			LOG(DEBUG) << "Starting CRC";
			auto crc = this->CrcInitialData(staticData, staticSize);
			LOG(DEBUG) << "Finish CRC: " << crc;
			this->addTex(statictexture, crc);
			//delete[] staticData;
		});
		Thread.detach();
	}

	//this is a terrible way to do this, dont do this
	std::vector<std::string> GetAccompaniedSkins(std::string originalfilename)
	{
		std::vector<std::string> toReturn;

		std::vector<std::string> searchFor;
		std::string filename = GetFilenameFromPath(originalfilename, true);
		std::string extension = GetFileExtension(originalfilename);
		//ANKY-0-1024-D
		std::string userSuppliedName = filename.substr(0, filename.length() - 13);//includes _

		std::string directory = GetFileDirectory(originalfilename);
		//ANKY-0-1024-N
		std::string id = filename.substr(filename.length() - 13, 13);
		std::string searchName;
		searchName = id.substr(0, 12) + "A";
		searchFor.push_back(directory + userSuppliedName + searchName + "." + extension);
		searchName = id.substr(0, 12) + "N";
		searchFor.push_back(directory + userSuppliedName + searchName + "." + extension);
		searchName = id.substr(0, 12) + "P";
		searchFor.push_back(directory + userSuppliedName + searchName + "." + extension);
		searchName = id.substr(0, 12) + "L";
		searchFor.push_back(directory + userSuppliedName + searchName + "." + extension);
		searchName = id.substr(0, 12) + "1";
		searchFor.push_back(directory + userSuppliedName + searchName + "." + extension);
		searchName = id.substr(0, 12) + "2";
		searchFor.push_back(directory + userSuppliedName + searchName + "." + extension);


		for (int i = 0; i < searchFor.size(); i++)
		{
			std::map<std::string, uint32_t>::iterator it = discoveredSkins.find(searchFor[i]);
			if (it != discoveredSkins.end())
			{
				toReturn.push_back(it->first);
			}
		}

		return toReturn;
	}

	void SaveTexture(int texIdToSave)
	{
		if (!savingImage)
		{
			LOG(DEBUG) << "==============SavingTexture...==============";

			savingImage = true;

			DirectX::ScratchImage image;




			if (singleContext == nullptr)
				LOG(WARNING) << "context is nullptr";
			else
			{

				if (originalDevice == nullptr)
				{
					LOG(WARNING) << "tempDevice is nullptr";
				}
				else
				{
					if (texIdToSave >= GetDebugTexList().size())
					{
						LOG(DEBUG) << "Texture out of range";
						return;
					}
					ID3D11Texture2D * tex = GetDebugTexList()[texIdToSave].get();
					D3D11_TEXTURE2D_DESC desc;
					tex->GetDesc(&desc);
					PrintDescription(desc);

					LOG(DEBUG) << "SaveTexture - CaptureTexture...";


					HRESULT hr = DirectX::CaptureTexture(originalDevice, singleContext, tex, image);
					//singleton_mutex.unlock();

					if (SUCCEEDED(hr))
					{
						LOG(DEBUG) << "Saving to memory..";


						PrintMetadata(image.GetMetadata());

						auto crc = GetDebugInitialCrcList()[texIdToSave];

						std::stringstream ss;
						if (CrcAsName)
						{
							ss << saveTextureDirectory << "\\" << texIdToSave << "_" << crc << "_.dds";
						}
						else
						{
							ss << saveTextureDirectory << "\\" << std::to_string(texIdToSave) << ".dds";
						}
						std::string dir = ss.str();
						std::wstring widestr = std::wstring(dir.begin(), dir.end());
						const wchar_t* cdar = widestr.c_str();

						LOG(DEBUG) << "Filename: " << cdar;

						LOG(DEBUG) << "SaveToDDSFile...";
						hr = SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
							DirectX::DDS_FLAGS_NONE, cdar);


						if (FAILED(hr))
						{
							LOG(WARNING) << "Could not SaveToDDSFile";
						}
						else
						{
							LOG(WARNING) << "Save Complete?";
						}
					}
					else
					{
						LOG(WARNING) << "Could not CaptureTexture";
					}

				}
			}

			savingImage = false;
		}
		else
		{
			LOG(DEBUG) << "Already savingImage for " << texIdToSave;
		}
		LOG(DEBUG) << "";
	}

	DirectX::ScratchImage LoadTextureFromFile(std::string filename, bool absoluteFile)
	{
		LOG(DEBUG) << "LoadTextureFromFile...";


		DirectX::ScratchImage image;

		LOG(DEBUG) << "fetching context...";
		ID3D11DeviceContext * context = singleContext;

		if (context == nullptr)
			LOG(WARNING) << "context is nullptr";
		else
		{
			ID3D11Device * tempDevice;
			context->GetDevice(&tempDevice);

			if (tempDevice == nullptr)
			{
				LOG(WARNING) << "tempDevice is nullptr";
			}
			else
			{
				std::stringstream ss;
				if (absoluteFile)
				{
					ss << filename;
				}
				else
				{
					ss << saveTextureDirectory + "\\" + filename;
				}

				std::string dir = ss.str();
				std::wstring widestr = std::wstring(dir.begin(), dir.end());
				const wchar_t* cdar = widestr.c_str();

				LOG(DEBUG) << "Filename: " << cdar;


				/*
				LOG(DEBUG) << "SaveToDDSFile...";
				hr = SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
					DirectX::DDS_FLAGS_NO_LEGACY_EXPANSION, cdar);
				*/
				//filename
				auto extension = GetFileExtension(filename);
				std::transform(extension.begin(), extension.end(), extension.begin(), ::toupper);
				if (extension == "DDS")
				{
					DirectX::ScratchImage loadedImage;

					DirectX::TexMetadata metadata;
					LOG(DEBUG) << "GetMetadataFromDDSFile...";


					HRESULT hr = DirectX::GetMetadataFromDDSFile(cdar,
						0, metadata);

					if (FAILED(hr))
					{
						LOG(WARNING) << "Could not GetMetadataFromDDSFile";
						return loadedImage;
					}

					PrintMetadata(metadata);

					LOG(DEBUG) << "LoadFromDDSFile...";
					hr = DirectX::LoadFromDDSFile(cdar, 0, &metadata, loadedImage);

					if (FAILED(hr))
					{
						LOG(WARNING) << "Could not LoadFromDDSFile";
						return loadedImage;
					}
					return loadedImage;
				}
				//.BMP, .PNG, .GIF, .TIFF, .JPEG
				else if (extension == "PNG" || extension == "BMP" || extension == "GIF" || extension == "TIFF" || extension == "JPEG")
				{
					DirectX::ScratchImage loadedImage;

					DirectX::TexMetadata metadata;
					LOG(DEBUG) << "GetMetadataFromWICFile...";

					HRESULT hr = DirectX::GetMetadataFromWICFile(cdar,
						0, metadata);

					if (FAILED(hr))
					{
						LOG(WARNING) << "Could GetMetadataFromWICFile";
						return loadedImage;
					}

					PrintMetadata(metadata);

					LOG(DEBUG) << "LoadFromWICFile...";
					hr = DirectX::LoadFromWICFile(cdar, 0, &metadata, loadedImage);

					if (FAILED(hr))
					{
						LOG(WARNING) << "Could not LoadFromWICFile";
						return loadedImage;
					}
					return loadedImage;
				}
			}
		}
	}

	void SaveTextureFromID(int id)
	{
		LOG(DEBUG) << "==LoadTextureFromID==";
		DirectX::ScratchImage tempImage;

		if (GetDebugTexList().size() <= id)
		{
			LOG(ERROR) << "id out of array!";
			return;
		}
		auto tex = GetDebugTexList()[id];

		if (tex == nullptr)
		{
			LOG(ERROR) << "tex is nullptr!";
			return;
		}

		LOG(DEBUG) << "--get desc...";
		D3D11_TEXTURE2D_DESC desc;
		tex.get()->GetDesc(&desc);

		LoadedImageWidth = desc.Width;
		LoadedImageHeight = desc.Height;

		ImageIsLoaded = true;


		DirectX::Blob tempBlob;
		LOG(DEBUG) << "--CaptureTexture...";

		HRESULT hr;
		try
		{
			//s_mutex.lock();
			hr = DirectX::CaptureTexture(originalDevice, singleContext, tex.get(), LoadedImage);
			//s_mutex.unlock();
		}
		catch (const std::overflow_error& e)
		{
			LOG(ERROR) << "--!CaptureTexture ERROR: " << e.what();
		}
		catch (const std::runtime_error& e)
		{
			LOG(ERROR) << "--!CaptureTexture ERROR: " << e.what();
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << "--!CaptureTexture ERROR: " << e.what();
		}
		catch (...)
		{
			LOG(ERROR) << "--!CaptureTexture unknown error";
		}

		LOG(DEBUG) << "--Saving to memory..";
		hr = DirectX::SaveToDDSMemory(LoadedImage.GetImages(), LoadedImage.GetImageCount(), LoadedImage.GetMetadata(),
			texFlags, tempBlob);

		if (FAILED(hr))
		{
			LOG(WARNING) << "--Could not SaveToDDSMemory";
			return;
		}


		return;

		/*
		LOG(DEBUG) << "--Saving to memory..";

		hr = DirectX::SaveToDDSMemory(tempImage.GetImages(), tempImage.GetImageCount(), tempImage.GetMetadata(),
			texFlags, tempBlob);

		if (FAILED(hr))
		{
			LOG(WARNING) << "--Could not SaveToDDSMemory";
			return tempImage;
		}
		*/

	}

	void PrintMetadata(DirectX::TexMetadata metadata)
	{

		LOG(DEBUG) << "--PrintMetadata--";
		LOG(DEBUG) << "arraySize: " << metadata.arraySize;
		LOG(DEBUG) << "depth: " << metadata.depth;
		LOG(DEBUG) << "dimension: " << metadata.dimension;
		LOG(DEBUG) << "format: " << metadata.format;
		LOG(DEBUG) << "height: " << metadata.height;
		LOG(DEBUG) << "mipLevels: " << metadata.mipLevels;
		LOG(DEBUG) << "miscFlags: " << metadata.miscFlags;
		LOG(DEBUG) << "miscFlags2: " << metadata.miscFlags2;
		LOG(DEBUG) << "width: " << metadata.width;
		LOG(DEBUG) << "-------------------";
	}
	void PrintDescription(D3D11_TEXTURE2D_DESC desc)
	{

		LOG(DEBUG) << "--+PrintDescription+--";
		LOG(DEBUG) << "ArraySize: " << (UINT)desc.ArraySize;
		LOG(DEBUG) << "BindFlags: " << (UINT)desc.BindFlags;
		LOG(DEBUG) << "CPUAccessFlags: " << (UINT)desc.CPUAccessFlags;
		LOG(DEBUG) << "Format: " << (UINT)desc.Format;
		LOG(DEBUG) << "Height: " << (UINT)desc.Height;
		LOG(DEBUG) << "MipLevels: " << (UINT)desc.MipLevels;
		LOG(DEBUG) << "MiscFlags: " << (UINT)desc.MiscFlags;
		//LOG(DEBUG) << "SampleDesc: " << desc.SampleDesc;
		LOG(DEBUG) << "Usage: " << (UINT)desc.Usage;
		LOG(DEBUG) << "Width: " << (UINT)desc.Width;
		LOG(DEBUG) << "-------------------";

	}

	void ResetDebugShaderList()
	{
		mtx.lock();
		DebugTexList.clear();
		DebugInitialCrcList.clear();
		mtx.unlock();
	}


	uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
	{
		int k;

		try
		{
			crc = ~crc;
			while (len--) {
				crc ^= *buf++;
				for (k = 0; k < 8; k++)
					crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
			}
		}
		catch (...)
		{
			LOG(WARNING) << "!!!!!!!!!!===crc32c catch==!!!!!!!!!";
			return 0;
		}


		return ~crc;
	}


	uint32_t CrcInitialData(const void *data, int size)
	{
		//LOG(INFO) << "--CrcInitialData--";

		if (!memory_readable(data, size))
		{
			LOG(WARNING) << "--Memory is not readable!--";
			return 0;
		}

		const unsigned char* ucharptr = reinterpret_cast<const unsigned char*>(data);
		uint32_t crc = 0;
		try
		{
			crc = crc32c(0, ucharptr, size);
		}
		catch (...)
		{
			LOG(WARNING) << "!!!!!!!!!!===Crc catch==!!!!!!!!!";
			return 0;
		}
		LOG(INFO) << "Initial Data CRC: " << crc;
		return crc;
	}

	//cant get this to work
	uint32_t PartialCrcInitialData(const void *data, int WidthOrHeight)
	{
		LOG(INFO) << "PartialCrcInitialData";

		const unsigned char* ucharptr = reinterpret_cast<const unsigned char*>(data);

		//a diagonal line towards the middle from a corner
		int to = WidthOrHeight / 2;

		std::vector<unsigned char> data2(to);
		for (int i = 0; i < to; i++)
		{
			data2[i] = ucharptr[(WidthOrHeight * i) + i];
		}


		const unsigned char* partialDataPointer = reinterpret_cast<const unsigned char*>(data2.data());

		/*
		unsigned char * partialData = new unsigned char[WidthOrHeight];
		for (int i = 0; i < to; i++)
		{
			partialData[i] = ucharptr[(WidthOrHeight * i) + i];
		}
		*/


		//auto ucharptrpart = reinterpret_cast<const unsigned char*>(partialData);

		uint32_t crc = 0;
		crc = crc32c(0, partialDataPointer, to);

		LOG(DEBUG) << "Partial Data CRC of size " << WidthOrHeight << ": " << crc;
		return crc;
	}


	bool memory_readable(const void *ptr, size_t byteCount)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery(ptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
			return false;

		if (mbi.State != MEM_COMMIT)
			return false;

		if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
			return false;

		// This checks that the start of memory block is in the same "region" as the
		// end. If it isn't you "simplify" the problem into checking that the rest of 
		// the memory is readable.
		size_t blockOffset = (size_t)((char *)ptr - (char *)mbi.AllocationBase);
		size_t blockBytesPostPtr = mbi.RegionSize - blockOffset;

		if (blockBytesPostPtr < byteCount)
			return memory_readable((char *)ptr + blockBytesPostPtr,
				byteCount - blockBytesPostPtr);

		return true;
	}


	void toClipboard(const std::string &s) {
		OpenClipboard(0);
		EmptyClipboard();
		HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size());
		if (!hg) {
			CloseClipboard();
			return;
		}
		memcpy(GlobalLock(hg), s.c_str(), s.size());
		GlobalUnlock(hg);
		SetClipboardData(CF_TEXT, hg);
		CloseClipboard();
		GlobalFree(hg);
	}
};