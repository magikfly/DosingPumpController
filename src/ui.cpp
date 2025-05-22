#include <Arduino.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "storage.h"
#include "dosing.h"

#define NUM_PUMPS 3
#define MAX_DOSES_PER_DAY 24
extern AsyncWebServer server(80); // or declare here if not global

void setupUI() {
    // Serve static files

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/favicon.svg", SPIFFS, "/favicon.svg");

    // --- GET /pump_status ---
server.on("/pump_status", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<4096> doc;
    for (int i = 0; i < NUM_PUMPS; ++i) {
        auto obj = doc.createNestedObject(String(i));
        obj["name"] = Storage::getPumpName(i);
        obj["calibration"] = Storage::getCalibration(i);
        obj["container"] = Storage::getContainerVolume(i);
        obj["remaining"] = Storage::getRemainingVolume(i);
        obj["dailyDose"] = Storage::getDailyDose(i);
        obj["dosesPerDay"] = Storage::getDosesPerDay(i);
        obj["enabled"] = Storage::isPumpEnabled(i);
        obj["activeDays"] = Storage::getPumpActiveDays(i);

        // Dose times
        JsonArray times = obj.createNestedArray("doseTimes");
        int doses = Storage::getDosesPerDay(i);
        for (int j = 0; j < doses; ++j) {
            times.add(Storage::getDoseTime(i, j));
        }
        JsonArray amts = obj.createNestedArray("doseAmounts");
            for (int j = 0; j < doses; ++j) {
            amts.add(Storage::getDoseAmount(i, j));
        }

        // Dose labels
        JsonArray labels = obj.createNestedArray("doseLabels");
        for (int j = 0; j < doses; ++j) {
            labels.add(Storage::getDoseLabel(i, j));
        }
        // Dose history
        JsonArray hist = obj.createNestedArray("history");
        Storage::getDoseHistory(i, hist);
    }
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
});

    // --- POST /api/set_bulk_settings ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_bulk_settings",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject doc = json.as<JsonObject>();
            int pump = doc["pump"] | 0;
            float vol = doc["vol"] | 0;
            float dose = doc["dailyDose"] | 0;
            int n = doc["dosesPerDay"] | 1;

            if (pump < 0 || pump >= NUM_PUMPS || n < 1 || n > MAX_DOSES_PER_DAY) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid value\"}");
                return;
            }

            int prevN = Storage::getDosesPerDay(pump);
            Storage::setContainerVolume(pump, vol);
            Storage::setDailyDose(pump, dose);
            Storage::setDosesPerDay(pump, n);

            if (n != prevN) {
                for (int i = 0; i < n; ++i) {
                    int mins = (i * 1440) / n;
                    Storage::setDoseTime(pump, i, mins);
                }
                for (int i = n; i < prevN && i < MAX_DOSES_PER_DAY; ++i) {
                    Storage::setDoseTime(pump, i, 0);
                }
            }

            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    // --- POST /api/set_pump_name ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_pump_name",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            String name = json["name"] | "";
            if (pump < 0 || pump >= NUM_PUMPS) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid value\"}");
                return;
            }
            Storage::setPumpName(pump, name);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    // --- POST /api/set_dose_time ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_dose_time",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            int dose = json["dose"] | 0;
            int mins = json["mins"] | 0;
            if (pump < 0 || pump >= NUM_PUMPS || dose < 0 || dose >= MAX_DOSES_PER_DAY) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid value\"}");
                return;
            }
            Storage::setDoseTime(pump, dose, mins);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    // --- POST /api/calibrate_start ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/calibrate_start",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            float ml = json["ml"] | 0;
            float mlps = Storage::getCalibration(pump);
            unsigned long ms = mlps > 0 ? (unsigned long)((ml / mlps) * 1000.0) : 0;
            Dosing::prime(pump, ms);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    // --- POST /api/calibrate_finish ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/calibrate_finish",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            float target = json["target"] | 0;
            float actual = json["actual"] | 0;
            float mlps = Storage::getCalibration(pump);
            float seconds = (mlps > 0) ? target / mlps : 1;
            if (actual > 0 && seconds > 0) {
                float new_mlps = actual / seconds;
                Storage::setCalibration(pump, new_mlps);
            }
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    // --- POST /api/dose_manual ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/dose_manual",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            float ml = json["ml"] | 0;
            float mlps = Storage::getCalibration(pump);
            unsigned long ms = mlps > 0 ? (unsigned long)((ml / mlps) * 1000.0) : 0;
            Dosing::prime(pump, ms);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    // --- POST /api/prime ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/prime_start",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            Dosing::primeStart(pump);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/prime_stop",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            int pump = json["pump"] | 0;
            Dosing::primeStop(pump);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));
    // --- Keep this for calibration or fixed-time actions ---


    // --- POST /api/set_wifi ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_wifi",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            String ssid = json["ssid"] | "";
            String password = json["password"] | "";
            Storage::setWiFiCredentials(ssid, password);
            request->send(200, "application/json", "{\"ok\":true}");
            delay(500); // let the response go
            ESP.restart();
        }
    ));

    // --- POST /api/set_timezone ---
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_timezone",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            String timezone = json["timezone"] | "";
            Storage::setTimezone(timezone);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_pump_enabled",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            Serial.println("handler hit");
            int pump = json["pump"] | 0;
            bool enabled = json["enabled"] | false;
            if (pump < 0 || pump >= NUM_PUMPS) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid value\"}");
                return;
            }
            Storage::setPumpEnabled(pump, enabled);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    ));

    server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_dose_amount",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
        int pump = json["pump"] | 0;
        int dose = json["dose"] | 0;
        float amount = json["amount"] | 0.0;
        if (pump < 0 || pump >= NUM_PUMPS || dose < 0 || dose >= MAX_DOSES_PER_DAY) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid value\"}");
            return;
        }
        Storage::setDoseAmount(pump, dose, amount);
        request->send(200, "application/json", "{\"ok\":true}");
    }
));
server.addHandler(new AsyncCallbackJsonWebHandler("/api/set_active_days",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
        int pump = json["pump"] | 0;
        int days = json["days"] | 0;
        if (pump < 0 || pump >= NUM_PUMPS) {
            request->send(400, "application/json", "{\"ok\":false}");
            return;
        }
        Storage::setPumpActiveDays(pump, days);
        request->send(200, "application/json", "{\"ok\":true}");
    }
));



    // --- Not found handler for debugging ---
    server.onNotFound([](AsyncWebServerRequest *request){
        Serial.print("[HTTP] Not found: ");
        Serial.println(request->url());
        request->send(404, "text/plain", "Not found");
    });


    server.begin();
}
