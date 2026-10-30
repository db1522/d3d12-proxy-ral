#pragma once

// ROG Ally Life fork: a small status file next to the game, so a tool (the UE5 Lighting Fix widget) can tell what the
// proxy did in the last session: whether the GPU counted as integrated, and above all whether the game asked for the
// non-local memory size, the value Unreal Engine 5 uses to decide that a GPU is integrated and turn Lumen off.
// A game that never asks can't be changed by the fix.

namespace dxgi_proxy {

class Status final
{
public:
	using Clock = std::chrono::steady_clock;

	static Status &instance();

	void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

	// An adapter was created: was it integrated, and are overrides active for it?
	void onAdapter(bool integrated, bool overrides_active);

	// The game queried the non-local memory segment; overridden = the proxy replaced the answer.
	// real_was_zero: the GPU's own answer, before any override, was 0. That's what makes Unreal treat a GPU as
	// integrated and turn Lumen off, so it shows whether the game needs the fix.
	void onNonLocalQuery(bool overridden, bool real_was_zero);
	void onLocalQuery() noexcept { ++local_queries_; }
	void onDescQuery() noexcept { ++desc_queries_; }
	void onOverridesDisabled(double after_seconds);

	// Seconds since the first non-local query, or -1 before it.
	[[nodiscard]] double secondsSinceFirstQuery() const;

	// Writes the file now (also called on process exit).
	void write();

private:
	Status();
	void maybeWrite();

	bool enabled_{true};
	const Clock::time_point started_{Clock::now()};
	SYSTEMTIME started_utc_{};
	std::atomic<int> integrated_{-1};
	std::atomic<bool> overrides_active_{false};
	std::atomic<unsigned long long> non_local_queries_{0};
	std::atomic<unsigned long long> non_local_overridden_{0};
	std::atomic<unsigned long long> non_local_real_zero_{0};
	std::atomic<unsigned long long> local_queries_{0};
	std::atomic<unsigned long long> desc_queries_{0};
	std::atomic<long long> first_query_ms_{-1};
	std::atomic<long long> last_query_ms_{-1};
	std::atomic<long long> disabled_after_ms_{-1};
	std::atomic<long long> last_write_ms_{-100000};
	std::mutex write_mutex_;
};

}  // namespace dxgi_proxy
