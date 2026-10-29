#pragma once

namespace dxgi_proxy {

class Config final
{
public:
	inline static const std::string sIniFilePath{"dxgi_proxy.ini"};

	struct Log final
	{
		bool enable{false};
	};

	struct Override final
	{
		bool only_igpu{false};
		std::optional<std::wstring> gpu_description;
		std::optional<UINT> gpu_vendor_id;
		std::optional<UINT> gpu_device_id;
		std::optional<SIZE_T> gpu_dedicated_memory_size;
		std::optional<SIZE_T> gpu_non_local_memory_size;
		std::optional<std::chrono::seconds> disable_after;
		// ROG Ally Life fork: like disable_after, but counted from the game's first non-local memory query instead of
		// adapter creation, so games with long launchers or shader compiles still get the overrides when they decide.
		std::optional<std::chrono::seconds> disable_after_first_query;
	};

	// ROG Ally Life fork: dxgi_proxy.status.json next to the game (see dxgi_proxy_status.h).
	struct StatusFile final
	{
		bool enable{true};
	};

	struct DXGI final
	{
		std::string dll_path;
		bool enable_factory_proxy{true};
		bool enable_adapter_proxy{true};
	};

	explicit Config(const std::string &ini_file_path = sIniFilePath);

	[[nodiscard]] const Log &logSection() const noexcept { return log_section; }
	[[nodiscard]] const DXGI &dxgiSection() const noexcept { return dxgi_section; }
	[[nodiscard]] const Override &overrideSection() const noexcept { return override_section; }
	[[nodiscard]] const StatusFile &statusSection() const noexcept { return status_section; }

	[[nodiscard]] std::string toString() const;

private:
	Log log_section;
	Override override_section;
	StatusFile status_section;
	DXGI dxgi_section;
};

}  // namespace dxgi_proxy
