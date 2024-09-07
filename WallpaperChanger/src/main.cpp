#include <iostream>
#include <fstream>
#include <Windows.h>
#include <filesystem>

#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

const int MAGIC_BYTES = 8;

json read_config(const std::string& filepath);
void write_config(const json& data, const std::string& filepath);
void set_wallpaper(const std::string& image_path);
void change_wallpaper(const std::string& filepath);
bool is_image(const std::string& image_path);
std::vector<std::string> read_directory(const std::string& directory_name);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	std::string config_path = "config.json";
	change_wallpaper(config_path);
	return 0;
}

json read_config(const std::string& filepath) {
	std::ifstream fin(filepath);
	if (fin.is_open())
		return json::parse(fin);

	json data = {
		{"index", 0},
		{"images_folder", ""}
	};

	write_config(data, filepath);
	MessageBox(NULL, L"please edit the config.json file", L"WallpaperChanger", MB_OK | MB_ICONINFORMATION);
	return data;
}

void write_config(const json& data, const std::string& filepath) {
	std::ofstream fout(filepath);
	fout << std::setw(4) << data << std::endl;
}

void set_wallpaper(const std::string& image_path) {
	std::wstring wstr = std::wstring(image_path.begin(), image_path.end());
	SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)(wchar_t*)wstr.c_str(), SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
}

bool is_image(const std::string& image_path) {

	std::ifstream file(image_path, std::ios::binary);
	if (file.fail()) return false;

	std::array<unsigned char, MAGIC_BYTES> buffer;
	file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

	// JPEG
	if (buffer[0] == 0xFF && buffer[1] == 0xD8) return true;

	// PNG (full 8-byte signature)
	if (buffer == std::array<unsigned char, 8>{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}) return true;

	// BMP
	if (buffer[0] == 0x42 && buffer[1] == 0x4D) return true;

	return false;
}

void change_wallpaper(const std::string& filepath) {
	json config = read_config(filepath);

	std::string images_folder = config["images_folder"];

	if (!fs::is_directory(config["images_folder"]))
		return;

	auto images = read_directory(images_folder);
	if (images.empty())
		return;

	int index = config["index"];
	index = index % images.size();
	std::string image_path = images[index];
	if (is_image(image_path)) {
		set_wallpaper(image_path);
	}
	else {
		std::wstring w_image_path = std::wstring(image_path.begin(), image_path.end());
		std::wstring message = L"Invalid image: " + w_image_path;
		MessageBox(NULL, message.c_str(), L"WallpaperChanger", MB_OK | MB_ICONEXCLAMATION);
		
	}

	config["index"] = (index + 1) % images.size();

	write_config(config, filepath);

}


std::vector<std::string> read_directory(const std::string& directory_path) {
	std::vector<std::string> v;
	for (const auto& entry : fs::directory_iterator(directory_path))
		v.emplace_back(entry.path().string());
	return v;
}