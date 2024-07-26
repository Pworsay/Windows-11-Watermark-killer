#include "include.hxx"

#include "util/file.hxx"
#include "util/memory.hxx"

void schedule_delete_task (const std::string& file_path) {
	std::println ("\nScheduling a task to delete an old file at computer startup..");

	const auto task_name = "DeleteShell32Prev";
	const auto command = std::format ("cmd.exe /c del {}", file_path);

	const auto create_task_cmd = std::format (
		R"(schtasks /create /tn {} /tr "{}" /sc once /st 00:00 /ri 1 /du 9999:59 /f)",
		task_name, command
	);

	system (create_task_cmd.c_str ());
}

auto startup () -> int {
	char system_folder [MAX_PATH] {};
	char appdata_folder [MAX_PATH] {};
	char username [250] {};
	DWORD username_size = sizeof (username);

	if (!GetSystemDirectoryA (system_folder, MAX_PATH) ||
		GetEnvironmentVariableA ("APPDATA", appdata_folder, MAX_PATH) == 0 ||
		!GetUserNameA (username, &username_size)) {
		std::cerr << "Failed to get parameters" << std::endl;
		return 1;
	}

	const auto my_folder = std::format ("{}\\Windows11WatermarkKiller", appdata_folder);
	if (!std::filesystem::exists (my_folder)) {
		std::filesystem::create_directory (my_folder);
	}

	const auto shell32_path = std::format ("{}\\shell32.dll", system_folder);
	const auto shell32_prev_path = std::format ("{}\\shell32-{:d}.dll", system_folder, static_cast<uintptr_t>(time (0)));
	const auto shell32_backup_path = std::format ("{}\\shell32.dll.bak", my_folder);
	const auto dst_shell32_path = std::format ("{}\\shell32.dll", my_folder);

	if (!std::filesystem::exists (shell32_backup_path)) {
		std::filesystem::copy_file (shell32_path, shell32_backup_path);
	}

	std::println ("Windows 11 Watermark killer\n");

	std::println ("Select option:");
	std::println ("| Restore (r) - Restore original shell32 file");
	std::println ("| Patch (p) - Patch shell32 file");

	char option {};
	std::cin >> option;

	if (option != 'r' && option != 'p') {
		std::cerr << "Invalid option" << std::endl;
		return 1;
	}

	std::println ("\nGetting the right to shell32.dll");
	system (std::format ("takeown /F {}", shell32_path).c_str ());
	system (std::format ("icacls {} /grant {}:F", shell32_path, username).c_str ());

	if (option == 'r') {
		std::filesystem::rename (shell32_path, shell32_prev_path);
		std::filesystem::copy_file (shell32_backup_path, shell32_path);
	} else if (option == 'p') {
		const auto file = util::load_file (shell32_backup_path);
		if (file.empty ()) {
			std::cerr << "Failed to load file" << std::endl;
			return 1;
		}

		/* CDesktopWatermark::s_DesktopBuildPaint */
		const auto pattern = util::search_pattern (util::memory_section_t {file.data (), file.size ()},
			"\x48\x89\x5C\x24\x20\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x70\xF9\xFF\xFF", "xxxxxxxxxxxxxxxxxxxxxxxx");
		if (!pattern) {
			std::cerr << "Failed to find pattern" << std::endl;
			return 1;
		}

		*pattern->as<uint8_t*> () = 0xC3;
		if (!util::save_file (dst_shell32_path, file)) {
			std::cerr << "Failed to save file" << std::endl;
			return 1;
		}

		std::filesystem::rename (shell32_path, shell32_prev_path);
		std::filesystem::copy_file (dst_shell32_path, shell32_path);
	} else {
		std::cerr << "Invalid option" << std::endl;
		return 1;
	}

	schedule_delete_task (shell32_prev_path);
	std::println ("\nPerfect! Reboot your PC to take effect");
	system ("shutdown /r /t 300");

	return 0;
}

auto main () -> int try {
	auto result = startup ();
	std::this_thread::sleep_for (std::chrono::seconds (5));
	return result;
} catch (const std::exception& e) {
	std::cerr << "Error happens: " << e.what () << std::endl;
	std::this_thread::sleep_for (std::chrono::seconds (5));
	return 1;
}
