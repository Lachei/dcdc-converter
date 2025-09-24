/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/ip4_addr.h"
#include "lwip/apps/mdns.h"

#include "FreeRTOS.h"
#include "task.h"

#include "log_storage.h"
#include "access_point.h"
#include "wifi_storage.h"
#include "webserver.h"
#include "usb_interface.h"
#include "settings.h"
#include "measurements.h"
#include "crypto_storage.h"
#include "ntp_client.h"
#include "pwm.h"
#include "converter_control.h"
#include "measure.h"

// the measure and control task are started with the same prio and same in-between delay to have
// stable controls

TaskHandle_t measure_task_handle;
TaskHandle_t control_task_handle;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(measure_task_handle, &xHigherPriorityTaskWoken);
    vTaskNotifyGiveFromISR(control_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return 0; // Don't repeat the alarm
}

void measure_task(void *) {
    for (;;) {
        add_alarm_in_us(100, alarm_callback, NULL, true);
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)); // at max wait 1ms for the alarm
        measure_and_update(realtime_data::Default());
    }
}

void control_task(void *) {
    uint64_t prev = time_us_64();
    float avg = 100.f;
    uint32_t counter{};
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)); // at max wait 1ms for the alarm
        uint64_t cur = time_us_64();
        avg = .99f * avg + .01f * (cur - prev);
        if (++counter % 10000 == 0) // should fire every second
            LogInfo("Yep, another call, diff: {}", avg);
        prev = cur;
    }
}

void usb_comm_task(void *) {
    LogInfo("Usb communication task");
    crypto_storage::Default();

    for (;;) {
	handle_usb_command();
    }
}

void wifi_search_task(void *) {
    LogInfo("Wifi task started");
    if (wifi_storage::Default().ssid_wifi.empty()) // onyl start the access point by default if no normal wifi connection is set
        access_point::Default().init();

    wifi_storage::Default().update_hostname();

    for (;;) {
        LogInfo("Wifi update loop");
        wifi_storage::Default().update_wifi_connection();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, wifi_storage::Default().wifi_connected);
        wifi_storage::Default().update_hostname();
        wifi_storage::Default().update_scanned();
        if (wifi_storage::Default().wifi_connected)
            ntp_client::Default().update_time();
        vTaskDelay(wifi_storage::Default().wifi_connected ? 5000: 1000);
    }
}


// task to initailize everything and only after initialization startin all other threads
// cyw43 init has to be done in freertos task because it utilizes freertos synchronization variables
void startup_task(void *) {
    LogInfo("Starting initialization");
    std::cout << "Starting initialization\n";
    if (cyw43_arch_init()) {
        for (;;) {
            vTaskDelay(1000);
            LogError("failed to initialize\n");
            std::cout << "failed to initialize arch (probably ram problem, increase ram size)\n";
        }
    }
    cyw43_arch_enable_sta_mode();
    Webserver().start();
    LogInfo("Ready, running http at {}", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    LogInfo("Initialization done");
    pwm::init();
    std::cout << "Initialization done, get all further info via the commands shown in 'help'\n";
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    TaskHandle_t task_usb_comm;
    TaskHandle_t task_update_wifi;
    auto err = xTaskCreate(usb_comm_task, "usb_comm", 512, NULL, 1, &task_usb_comm);	// usb task also has to be started only after cyw43 init as some wifi functions are available
    if (err != pdPASS)
        LogError("Failed to start usb communication task with code {}" ,err);
    err = xTaskCreate(wifi_search_task, "UpdateWifiThread", 512, NULL, 1, &task_update_wifi);
    if (err != pdPASS)
        LogError("Failed to start wifi task with code {}" ,err);
    err = xTaskCreate(measure_task, "MeasureThread", 512, NULL, configMAX_PRIORITIES - 1, &measure_task_handle);
    if (err != pdPASS)
        LogError("Failed to start measure task with code {}" ,err);
    err = xTaskCreate(control_task, "MeasureThread", 512, NULL, configMAX_PRIORITIES - 1, &control_task_handle);
    if (err != pdPASS)
        LogError("Failed to start measure task with code {}" ,err);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    vTaskDelete(NULL);
}

int main( void )
{
    stdio_init_all();

    LogInfo("Starting FreeRTOS on all cores.");
    std::cout << "Starting FreeRTOS on all cores\n";

    TaskHandle_t task_startup;
    xTaskCreate(startup_task, "StartupThread", 512, NULL, 1, &task_startup);

    vTaskStartScheduler();
    return 0;
}
