#pragma once

#include <chrono> 
#include <limits>
#include <deque>
#include <memory>

using namespace std::chrono_literals;

//  A simple class to measure Frames Per Second (FPS) with a configurable update interval.
class fps_meter {
public:
	// Do not allow type conversion from integers, bool, float etc.
	explicit fps_meter() {}

	struct frame_capture {
		std::chrono::duration<double> frame_time;
		std::chrono::steady_clock::time_point capture_time;
	};


	// Get last FPS value without modification
	double get_current(void) { return m_fps; }
	double get_min(void) { return m_fps_min; }
	double get_max(void) { return m_fps_max; }

	// Call once per frame (end of the frame).
	void update(void) {
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> delta = now - m_last_time;
		m_last_time = now;

		auto cutoff = std::chrono::steady_clock::now() - std::chrono::seconds(1);

		while (frame_captures.size() > 0) {
			std::shared_ptr<frame_capture> fc = frame_captures.front();

			if (fc->capture_time < cutoff) {
				frame_captures.pop_front();
			} else {

				break;
			}
		}

		frame_captures.push_back(std::make_shared<frame_capture>(delta, now));

		long long ft_min = std::numeric_limits<long long>::max();
		long long ft_max = std::numeric_limits<long long>::lowest();
		long long ft_total = 0;

		size_t size = frame_captures.size();
		for (size_t i = 0; i < size; i++) {
			std::shared_ptr<frame_capture> fc = frame_captures.at(i);

			long long frame_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fc->frame_time).count();

			ft_total += frame_microseconds;

			if (frame_microseconds < ft_min) {
				ft_min = frame_microseconds;
			}
			if (frame_microseconds > ft_max) {
				ft_max = frame_microseconds;
			}
		}

		m_fps = size / (ft_total / 1000000.0);
		m_fps_min = 1000000.0 / ft_max;
		m_fps_max = 1000000.0 / ft_min;
	}

	// Restart frame counting
	void reset(void) {
		m_last_time = std::chrono::steady_clock::now();
		m_fps = 0.0;
		m_fps_min = 0.0;
		m_fps_max = 0.0;
		frame_captures.clear();
	}
private:

	std::deque<std::shared_ptr<frame_capture>> frame_captures;

	std::chrono::time_point<std::chrono::steady_clock> m_last_time = std::chrono::steady_clock::now();

	double m_fps{ 0.0 };
	double m_fps_min{ 0.0 };
	double m_fps_max{ 0.0 };
};
