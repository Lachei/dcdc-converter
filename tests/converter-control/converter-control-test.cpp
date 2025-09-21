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
	const float bat_max_ws{100};
	std::atomic<double> bat_cur_ws{bat_max_ws/2};
	std::atomic<float> bat_cur_amp{};
	std::atomic<float> high_cur_amp{};
	std::atomic<float> high_v{450};
	std::atomic<float> v_should_low{};
	std::atomic<bool> enable_battery_sim{true};
	
	static sim_state& Default() { static sim_state state{}; return state; }
	void update(float delta_ms, float duty_cycle) {
		// the duty cyle sets the high to low side voltage ratio that would be achieved in steady state
		// dc_ratio_lh = volt_low/volt_high;
		float dc_ratio_lh = (1.f - duty_cycle);
		v_should_low = dc_ratio_lh * high_v;
		float bat_v_diff = bat_max_v - bat_min_v;
		double bat_cur_v = (bat_cur_ws / bat_max_ws) * bat_v_diff + bat_min_v;
		bat_cur_amp = (v_should_low - bat_cur_v) / bat_inner_ohm;
		if (enable_battery_sim)
			bat_cur_ws += bat_cur_amp * delta_ms / (1000.);  // divide by 1k for k, by 3600 for hours conversion
	}
	void sync_to_realtime_data() const {
		realtime_data &rt = realtime_data::Default();
		float bat_v_diff = bat_max_v - bat_min_v;
		rt.low_side_v = (bat_cur_ws / bat_max_ws) * bat_v_diff + bat_min_v;
		rt.high_side_v = high_v;
		rt.low_side_a = bat_cur_amp;
		rt.high_side_a = bat_cur_amp * rt.low_side_v / high_v;
	}
};

void prime_err_int() {
	if (settings::Default().k_i == 0)
		return;
	float target_dc = ((1.f - 1.f / settings::Default().high_to_low_ratio) - .5) * 2;
	if (target_dc < 0)
		realtime_data::Default().error_integral = (target_dc / (1 + target_dc)) / settings::Default().k_i;
	else
		realtime_data::Default().error_integral = (target_dc / (1 - target_dc)) / settings::Default().k_i;
	// target_dc = x / (1 + abs(x));
	// target_dc + target_dc * abs(x) = x;
	// 1. x < 0 -> abs(x) = -x
	//	target_dc = (1 + target_dc)x; -> x = target_dc / (1 + target_dc)
	// 2. x >= 0 -> abs(x) = x
	//	target_dc = (1 - target_dc)x; -> x = target_dc / (1 - target_dc)
}

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
	std::cout << "  kp ${p constant}" << std::endl;
	std::cout << "  ki ${i constant}" << std::endl;
	std::cout << "  kd ${d constant}" << std::endl;
	std::cout << "  hv ${HIGH_SIDE_VOLTAGE}" << std::endl;
	std::cout << "  eb ${0|1} # enable battery sim" << std::endl;
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
			} else if (cmd == "kp") {
				std::cin >> settings::Default().k_p;
			} else if (cmd == "ki") {
				std::cin >> settings::Default().k_i;
			} else if (cmd == "kd") {
				std::cin >> settings::Default().k_d;
			} else if (cmd == "eb") {
				bool t;
				std::cin >> t; 
				sim_state::Default().enable_battery_sim = t;
			} else if (cmd == "q") {
				std::cout << "Got quit command" << std::endl;
				continue_running = false;
			} else {
				std::cerr << "Invalid command " << cmd << std::endl;
			}
		}
	});

	std::thread data_write = std::thread([&]{
		res << "# time high_side_v bat_cur_ws bat_cur_amp bat_cur_v duty_cycle target_low_v err_amp goal_amp" << std::endl;
		// write out every second a datapoint
		const sim_state &sim = sim_state::Default();
		const realtime_data &rd = realtime_data::Default();
		int second{};
		while(continue_running) {
			float bat_v_diff = sim.bat_max_v - sim.bat_min_v;
			res << ++second / 100. << ' ' << sim.high_v << ' ' << sim.bat_cur_ws << ' ' << sim.bat_cur_amp << ' ' << sim.bat_cur_ws / sim.bat_max_ws * bat_v_diff + sim.bat_min_v << ' ' << realtime_data::Default().duty_cycle * 1000 << ' ' << sim.v_should_low << ' ' << rd.err << ' ' << rd.goal_amp << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	});

	std::thread wild_voltage = std::thread([&]{
		int a{};
		while(continue_running) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			if (a)
				sim_state::Default().high_v = 500;
			else
				sim_state::Default().high_v = 400;
			a ^= 1;
		}
	});

	prime_err_int();
	auto prev_time = std::chrono::steady_clock::now();
	while (continue_running) {
		sim_state::Default().sync_to_realtime_data();
		auto cur_time = std::chrono::steady_clock::now();
		double delta_ms = std::chrono::duration_cast<std::chrono::microseconds>(cur_time - prev_time).count() * 1e-3;
		prev_time = cur_time;
		float duty_cycle = control_step(delta_ms);
		sim_state::Default().update(delta_ms, duty_cycle);
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	input.join();
	data_write.join();
	wild_voltage.join();
	return EXIT_SUCCESS;
}

