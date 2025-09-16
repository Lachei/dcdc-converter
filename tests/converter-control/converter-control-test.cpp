#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include "../../include/converter_control.h"

struct sim_state {
	const float bat_inner_ohm{.1};
	const float bat_max_v{170};
	const float bat_min_v{130};
	const float bat_max_kwh{30};
	std::atomic<float> bat_cur_kwh{15};
	std::atomic<float> bat_cur_amp{};
	std::atomic<float> high_cur_amp{};
	std::atomic<float> high_v{450};
	
	static sim_state& Default() { static sim_state state{}; return state; }
	void update(float delta_seconds, float duty_cycle) {
		// the duty cyle sets the high to low side voltage ratio that would be achieved in steady state
		// dc_ratio_lh = volt_low/volt_high;
		float dc_ratio_lh = (1 - duty_cycle);
		float v_should_low = dc_ratio_lh * high_v;
		float bat_v_diff = bat_max_v - bat_min_v;
		float bat_cur_v = bat_cur_kwh / bat_max_kwh * bat_v_diff + bat_min_v;
		bat_cur_amp = (v_should_low - bat_cur_v) / bat_inner_ohm;
		bat_cur_kwh += bat_cur_amp * delta_seconds / (1000 * 3600);  // divide by 1k for k, by 3600 for hours conversion
	}
	void sync_to_realtime_data() const {
		realtime_data &rt = realtime_data::Default();
		float bat_v_diff = bat_max_v - bat_min_v;
		rt.low_side_v = bat_cur_kwh / bat_max_kwh * bat_v_diff + bat_min_v;
		rt.high_side_v = high_v;
		rt.low_side_a = bat_cur_amp;
		rt.high_side_a = bat_cur_amp * rt.low_side_v / high_v;
	}
};

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Binary has to be called as follows: converter-control-test ${OUT_FILE}" << std::endl;
		return EXIT_FAILURE;
	}

	if (std::filesystem::exists(argv[1])) {
		std::cout << "[CARE] Output file" << argv[1] << 
			" already exists. Are you sure you want to continue? Enter y for continue: ";
		std::string answer{};
		std::cin >> answer;
		std::cout << std::endl;
		if (answer != "y")
			return EXIT_FAILURE;
	}

	std::ofstream res{argv[1]};
	if (!res) {
		std::cerr << "Could not open out file, exiting" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Starting converter-control-test with output values being put in " << argv[1] << std::endl;
	std::cout << "To control the simulation enter one of the following commands:" << std::endl;
	std::cout << "  hv ${HIGH_SIDE_VOLTAGE}" << std::endl;
	std::cout << "  q # stop the simulation" << std::endl;

	std::atomic<bool> continue_running{true};
	std::thread input = std::thread([&]{
		std::string cmd;
		float v;
		while (continue_running) {
			std::cin >> cmd;
			if (cmd == "hv") {
				std::cin >> v;
				sim_state::Default().high_v = v;
			} else if (cmd == "q") {
				std::cout << "Got quit command" << std::endl;
				continue_running = false;
			} else {
				std::cerr << "Invalid command " << cmd << std::endl;
			}
		}
	});

	std::thread data_write = std::thread([&]{
		res << "# time high_side_v bat_cur_kwh bat_cur_amp" << std::endl;
		// write out every second a datapoint
		const sim_state &sim = sim_state::Default();
		int second{};
		while(continue_running) {
			res << ++second / 10. << ' ' << sim.high_v << ' ' << sim.bat_cur_kwh << ' ' << sim.bat_cur_amp << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
	
	auto prev_time = std::chrono::steady_clock::now();
	while (continue_running) {
		sim_state::Default().sync_to_realtime_data();
		auto cur_time = std::chrono::steady_clock::now();
		double delta = std::chrono::duration_cast<std::chrono::microseconds>(cur_time - prev_time).count() * 1e-6;
		float duty_cycle = control_step(delta);
		sim_state::Default().update(duty_cycle, delta);
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	input.join();
	data_write.join();
	return EXIT_SUCCESS;
}

