#pragma once

#include <chrono> 
#include <limits>
#include <deque>
#include <memory>
#include <numeric>
#include <algorithm>

using namespace std::chrono_literals;

//  A simple class to measure Frames Per Second (FPS) with a configurable update interval.
class fps_meter {
public:
	// Do not allow type conversion from integers, bool, float etc.
	explicit fps_meter(std::chrono::duration<double> interval = 1.0s) : m_interval(interval) {}

	struct frame_capture {
		std::chrono::duration<double> frame_time;
		std::chrono::steady_clock::time_point capture_time;
	};

	// Get last FPS value without modification
	double get_current(void) { return m_fps; }
	double get_min(void) { return m_fps_min; }
	double get_max(void) { return m_fps_max; }
	double get_1_low() { return m_fps_1_low; }
	double get_01_low() { return m_fps_01_low; }

	// Call once per frame (end of the frame).
	void update(void) {
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> delta = now - m_last_time;

		auto cutoff = now - m_interval;

		while (!m_frames.empty() && m_frames.front().capture_time < cutoff) {
			m_frames.pop_front();
		}

		m_frames.push_back({ delta, now });

		const size_t n = m_frames.size();
		if (n == 0) return;

		// prepare samples for percentiles
		m_samples.clear();
		m_samples.reserve(n);

		long long ft_min = std::numeric_limits<long long>::max();
		long long ft_max = std::numeric_limits<long long>::lowest();
		long long ft_total = 0;

		size_t size = m_frames.size();
		for (const auto& fc : m_frames) {
			long long us = std::chrono::duration_cast<std::chrono::microseconds>(fc.frame_time).count();
			m_samples.push_back(us);

			ft_total += us;
			ft_min = std::min(ft_min, us);
			ft_max = std::max(ft_max, us);
		}


		// average FPS
		double total_sec = ft_total / 1'000'000.0;
		m_fps = n / total_sec;

		// min/max FPS
		m_fps_max = 1.0 / (ft_min / 1'000'000.0); // fastest frame
		m_fps_min = 1.0 / (ft_max / 1'000'000.0); // slowest frame

		// percentiles
		compute_percentile(m_samples, 0.99, m_fps_1_low);   // 1% low
		compute_percentile(m_samples, 0.999, m_fps_01_low); // 0.1% low

		m_last_time = now;
	}

	// Restart frame counting
	void reset(void) {
		m_frames.clear();

		m_last_time = std::chrono::steady_clock::now();

		m_fps = 0.0;
		m_fps_min = 0.0;
		m_fps_max = 0.0;
		m_fps_1_low = 0.0;
		m_fps_01_low = 0.0;
	}
private:
	void compute_percentile(std::vector<long long>& data, double percentile, double& out_fps) {
		if (data.empty()) {
			out_fps = 0.0;
			return;
		}

		size_t n = data.size();
		size_t idx = static_cast<size_t>(n * percentile);
		if (idx >= n) idx = n - 1;

		// partition (O(n))
		std::nth_element(data.begin(), data.begin() + idx, data.end());

		// average worst tail for stability
		long long sum = 0;
		size_t count = 0;

		for (size_t i = idx; i < n; ++i) {
			sum += data[i];
			count++;
		}

		double avg_us = sum / (double)count;
		out_fps = 1.0 / (avg_us / 1'000'000.0);
	}

	std::deque<frame_capture> m_frames;
	std::vector<long long> m_samples;

	std::chrono::time_point<std::chrono::steady_clock> m_last_time = std::chrono::steady_clock::now();

	std::chrono::duration<double> m_interval = 1.0s;

	double m_fps{ 0.0 };
	double m_fps_min{ 0.0 };
	double m_fps_max{ 0.0 };
	double m_fps_1_low{ 0.0 };
	double m_fps_01_low{ 0.0 };
};
