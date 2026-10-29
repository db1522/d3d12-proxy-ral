#include "pch.h"

namespace dxgi_proxy {

namespace {

constexpr long long kWriteEveryMs = 5000;
constexpr const char *kVersion = "1.0";

std::wstring exeDirectory()
{
	wchar_t path[MAX_PATH * 4]{};
	const auto n = GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
	std::wstring s(path, n);
	const auto slash = s.find_last_of(L"\\/");
	return slash == std::wstring::npos ? std::wstring{} : s.substr(0, slash + 1);
}

std::string exeName()
{
	char path[MAX_PATH * 4]{};
	const auto n = GetModuleFileNameA(nullptr, path, static_cast<DWORD>(std::size(path)));
	std::string s(path, n);
	const auto slash = s.find_last_of("\\/");
	return slash == std::string::npos ? s : s.substr(slash + 1);
}

std::string isoTime(const SYSTEMTIME &t)
{
	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02uZ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	return buffer;
}

std::string jsonEscape(const std::string &s)
{
	std::string out;
	for(const char c : s) {
		if(c == '"' || c == '\\') {
			out += '\\';
			out += c;
		} else if(static_cast<unsigned char>(c) < 0x20) {
			out += ' ';
		} else {
			out += c;
		}
	}
	return out;
}

std::string seconds(long long ms)
{
	if(ms < 0) {
		return "null";
	}
	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(ms) / 1000.0);
	return buffer;
}

}  // namespace

Status &Status::instance()
{
	static Status sInstance;
	return sInstance;
}

Status::Status()
{
	GetSystemTime(&started_utc_);
}

void Status::onAdapter(bool integrated, bool overrides_active)
{
	// The first adapter is the one games use; a later integrated one doesn't override an earlier dedicated one.
	int expected = -1;
	integrated_.compare_exchange_strong(expected, integrated ? 1 : 0);
	if(overrides_active) {
		overrides_active_ = true;
	}
}

void Status::onNonLocalQuery(bool overridden)
{
	const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count();
	long long expected = -1;
	const bool first = first_query_ms_.compare_exchange_strong(expected, now);
	last_query_ms_ = now;
	++non_local_queries_;
	if(overridden) {
		++non_local_overridden_;
	}
	if(first) {
		write();
	} else {
		maybeWrite();
	}
}

void Status::onOverridesDisabled(double after_seconds)
{
	disabled_after_ms_ = static_cast<long long>(after_seconds * 1000.0);
	write();
}

double Status::secondsSinceFirstQuery() const
{
	const auto first = first_query_ms_.load();
	if(first < 0) {
		return -1.0;
	}
	const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count();
	return static_cast<double>(now - first) / 1000.0;
}

void Status::maybeWrite()
{
	const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count();
	if(now - last_write_ms_.load() >= kWriteEveryMs) {
		write();
	}
}

void Status::write()
{
	if(!enabled_) {
		return;
	}
	std::unique_lock lock{write_mutex_, std::try_to_lock};
	if(!lock.owns_lock()) {
		return;
	}
	last_write_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count();

	SYSTEMTIME now_utc{};
	GetSystemTime(&now_utc);
	const auto integrated = integrated_.load();

	std::string json;
	json += "{\n";
	json += "  \"tool\": \"d3d12-proxy (ROG Ally Life fork)\",\n";
	json += std::string("  \"version\": \"") + kVersion + "\",\n";
	json += "  \"process\": \"" + jsonEscape(exeName()) + "\",\n";
	json += "  \"started\": \"" + isoTime(started_utc_) + "\",\n";
	json += "  \"updated\": \"" + isoTime(now_utc) + "\",\n";
	json += std::string("  \"integratedGpu\": ") + (integrated < 0 ? "null" : integrated ? "true" : "false") + ",\n";
	json += std::string("  \"overridesActive\": ") + (overrides_active_ ? "true" : "false") + ",\n";
	json += "  \"nonLocalQueries\": " + std::to_string(non_local_queries_.load()) + ",\n";
	json += "  \"nonLocalOverridden\": " + std::to_string(non_local_overridden_.load()) + ",\n";
	json += "  \"localQueries\": " + std::to_string(local_queries_.load()) + ",\n";
	json += "  \"descQueries\": " + std::to_string(desc_queries_.load()) + ",\n";
	json += "  \"firstQuerySeconds\": " + seconds(first_query_ms_.load()) + ",\n";
	json += "  \"lastQuerySeconds\": " + seconds(last_query_ms_.load()) + ",\n";
	json += "  \"overridesStoppedAfterSeconds\": " + seconds(disabled_after_ms_.load()) + "\n";
	json += "}\n";

	// Write a temporary file and swap it in, so a reader never sees half a file. Plain Win32 calls: this also runs
	// while the process is exiting.
	const auto dir = exeDirectory();
	const auto final_path = dir + L"dxgi_proxy.status.json";
	const auto temp_path = dir + L"dxgi_proxy.status.json.tmp";
	const auto file = CreateFileW(temp_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE) {
		return;
	}
	DWORD written = 0;
	const bool ok = WriteFile(file, json.data(), static_cast<DWORD>(json.size()), &written, nullptr) && written == json.size();
	CloseHandle(file);
	if(ok) {
		MoveFileExW(temp_path.c_str(), final_path.c_str(), MOVEFILE_REPLACE_EXISTING);
	} else {
		DeleteFileW(temp_path.c_str());
	}
}

}  // namespace dxgi_proxy
