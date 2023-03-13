// ESP-IDF Compatibility
#pragma once

#include <map>
#include <utility>

#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/controller.h"
#include "esphome/core/component.h"

namespace esphome {
namespace prometheus {

class PrometheusHandler : public AsyncWebHandler, public Component {
 public:
  PrometheusHandler(web_server_base::WebServerBase *base) : base_(base) {};
  
  void handleRequest(AsyncWebServerRequest *req) override
  {
	  AsyncResponseStream *stream = req->beginResponseStream("text/plain; version=0.0.4; charset=utf-8");

	#ifdef USE_SENSOR
	  this->sensor_type_(stream);
	  for (auto *obj : App.get_sensors())
		this->sensor_row_(stream, obj);
	#endif

	#ifdef USE_BINARY_SENSOR
	  this->binary_sensor_type_(stream);
	  for (auto *obj : App.get_binary_sensors())
		this->binary_sensor_row_(stream, obj);
	#endif

	#ifdef USE_FAN
	  this->fan_type_(stream);
	  for (auto *obj : App.get_fans())
		this->fan_row_(stream, obj);
	#endif

	#ifdef USE_LIGHT
	  this->light_type_(stream);
	  for (auto *obj : App.get_lights())
		this->light_row_(stream, obj);
	#endif

	#ifdef USE_COVER
	  this->cover_type_(stream);
	  for (auto *obj : App.get_covers())
		this->cover_row_(stream, obj);
	#endif

	#ifdef USE_SWITCH
	  this->switch_type_(stream);
	  for (auto *obj : App.get_switches())
		this->switch_row_(stream, obj);
	#endif

	#ifdef USE_LOCK
	  this->lock_type_(stream);
	  for (auto *obj : App.get_locks())
		this->lock_row_(stream, obj);
	#endif

	  req->send(stream);
	}

  /** Determine whether internal components should be exported as metrics.
   * Defaults to false.
   *
   * @param include_internal Whether internal components should be exported.
   */
  void set_include_internal(bool include_internal) { include_internal_ = include_internal; }

  /** Add the value for an entity's "id" label.
   *
   * @param obj The entity for which to set the "id" label
   * @param value The value for the "id" label
   */
  void add_label_id(EntityBase *obj, const std::string &value) { relabel_map_id_.insert({obj, value}); }

  /** Add the value for an entity's "name" label.
   *
   * @param obj The entity for which to set the "name" label
   * @param value The value for the "name" label
   */
  void add_label_name(EntityBase *obj, const std::string &value) { relabel_map_name_.insert({obj, value}); }

  bool canHandle(AsyncWebServerRequest *request) override {
    if (request->method() == HTTP_GET) {
      if (request->url() == "/metrics")
        return true;
    }

    return false;
  }

  void setup() override {
    this->base_->init();
    this->base_->add_handler(this);
  }
  float get_setup_priority() const override {
    // After WiFi
    return setup_priority::WIFI - 1.0f;
  }

 protected:
  std::string relabel_id_(EntityBase *obj)
  {
	  auto item = relabel_map_id_.find(obj);
	  return item == relabel_map_id_.end() ? obj->get_object_id() : item->second;
  }

  std::string relabel_name_(EntityBase *obj)
  {
	  auto item = relabel_map_name_.find(obj);
	  return item == relabel_map_name_.end() ? obj->get_name() : item->second;
  }

	// Type-specific implementation
	#ifdef USE_SENSOR
	void sensor_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_sensor_value GAUGE\n"));
	  stream->print(F("#TYPE esphome_sensor_failed GAUGE\n"));
	}
	void sensor_row_(AsyncResponseStream *stream, sensor::Sensor *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  if (!std::isnan(obj->state)) {
		// We have a valid value, output this value
		stream->print(F("esphome_sensor_failed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} 0\n"));
		// Data itself
		stream->print(F("esphome_sensor_value{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\",unit=\""));
		stream->print(obj->get_unit_of_measurement().c_str());
		stream->print(F("\"} "));
		stream->print(value_accuracy_to_string(obj->state, obj->get_accuracy_decimals()).c_str());
		stream->print("\n");
	  } else {
		// Invalid state
		stream->print(F("esphome_sensor_failed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} 1\n"));
	  }
	}
	#endif

	// Type-specific implementation
	#ifdef USE_BINARY_SENSOR
	void binary_sensor_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_binary_sensor_value GAUGE\n"));
	  stream->print(F("#TYPE esphome_binary_sensor_failed GAUGE\n"));
	}
	void binary_sensor_row_(AsyncResponseStream *stream, binary_sensor::BinarySensor *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  if (obj->has_state()) {
		// We have a valid value, output this value
		stream->print(F("esphome_binary_sensor_failed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} 0\n"));
		// Data itself
		stream->print(F("esphome_binary_sensor_value{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} "));
		stream->print(obj->state);
		stream->print("\n");
	  } else {
		// Invalid state
		stream->print(F("esphome_binary_sensor_failed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} 1\n"));
	  }
	}
	#endif

	#ifdef USE_FAN
	void fan_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_fan_value GAUGE\n"));
	  stream->print(F("#TYPE esphome_fan_failed GAUGE\n"));
	  stream->print(F("#TYPE esphome_fan_speed GAUGE\n"));
	  stream->print(F("#TYPE esphome_fan_oscillation GAUGE\n"));
	}
	void fan_row_(AsyncResponseStream *stream, fan::Fan *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  stream->print(F("esphome_fan_failed{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} 0\n"));
	  // Data itself
	  stream->print(F("esphome_fan_value{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} "));
	  stream->print(obj->state);
	  stream->print("\n");
	  // Speed if available
	  if (obj->get_traits().supports_speed()) {
		stream->print(F("esphome_fan_speed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} "));
		stream->print(obj->speed);
		stream->print("\n");
	  }
	  // Oscillation if available
	  if (obj->get_traits().supports_oscillation()) {
		stream->print(F("esphome_fan_oscillation{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} "));
		stream->print(obj->oscillating);
		stream->print("\n");
	  }
	}
	#endif

	#ifdef USE_LIGHT
	void light_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_light_state GAUGE\n"));
	  stream->print(F("#TYPE esphome_light_color GAUGE\n"));
	  stream->print(F("#TYPE esphome_light_effect_active GAUGE\n"));
	}
	void light_row_(AsyncResponseStream *stream, light::LightState *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  // State
	  stream->print(F("esphome_light_state{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} "));
	  stream->print(obj->remote_values.is_on());
	  stream->print(F("\n"));
	  // Brightness and RGBW
	  light::LightColorValues color = obj->current_values;
	  float brightness, r, g, b, w;
	  color.as_brightness(&brightness);
	  color.as_rgbw(&r, &g, &b, &w);
	  stream->print(F("esphome_light_color{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\",channel=\"brightness\"} "));
	  stream->print(brightness);
	  stream->print(F("\n"));
	  stream->print(F("esphome_light_color{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\",channel=\"r\"} "));
	  stream->print(r);
	  stream->print(F("\n"));
	  stream->print(F("esphome_light_color{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\",channel=\"g\"} "));
	  stream->print(g);
	  stream->print(F("\n"));
	  stream->print(F("esphome_light_color{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\",channel=\"b\"} "));
	  stream->print(b);
	  stream->print(F("\n"));
	  stream->print(F("esphome_light_color{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\",channel=\"w\"} "));
	  stream->print(w);
	  stream->print(F("\n"));
	  // Effect
	  std::string effect = obj->get_effect_name();
	  if (effect == "None") {
		stream->print(F("esphome_light_effect_active{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\",effect=\"None\"} 0\n"));
	  } else {
		stream->print(F("esphome_light_effect_active{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\",effect=\""));
		stream->print(effect.c_str());
		stream->print(F("\"} 1\n"));
	  }
	}
	#endif

	#ifdef USE_COVER
	void cover_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_cover_value GAUGE\n"));
	  stream->print(F("#TYPE esphome_cover_failed GAUGE\n"));
	}
	void cover_row_(AsyncResponseStream *stream, cover::Cover *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  if (!std::isnan(obj->position)) {
		// We have a valid value, output this value
		stream->print(F("esphome_cover_failed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} 0\n"));
		// Data itself
		stream->print(F("esphome_cover_value{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} "));
		stream->print(obj->position);
		stream->print("\n");
		if (obj->get_traits().get_supports_tilt()) {
		  stream->print(F("esphome_cover_tilt{id=\""));
		  stream->print(relabel_id_(obj).c_str());
		  stream->print(F("\",name=\""));
		  stream->print(relabel_name_(obj).c_str());
		  stream->print(F("\"} "));
		  stream->print(obj->tilt);
		  stream->print("\n");
		}
	  } else {
		// Invalid state
		stream->print(F("esphome_cover_failed{id=\""));
		stream->print(relabel_id_(obj).c_str());
		stream->print(F("\",name=\""));
		stream->print(relabel_name_(obj).c_str());
		stream->print(F("\"} 1\n"));
	  }
	}
	#endif

	#ifdef USE_SWITCH
	void switch_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_switch_value GAUGE\n"));
	  stream->print(F("#TYPE esphome_switch_failed GAUGE\n"));
	}
	void switch_row_(AsyncResponseStream *stream, switch_::Switch *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  stream->print(F("esphome_switch_failed{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} 0\n"));
	  // Data itself
	  stream->print(F("esphome_switch_value{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} "));
	  stream->print(obj->state);
	  stream->print("\n");
	}
	#endif

	#ifdef USE_LOCK
	void lock_type_(AsyncResponseStream *stream) {
	  stream->print(F("#TYPE esphome_lock_value GAUGE\n"));
	  stream->print(F("#TYPE esphome_lock_failed GAUGE\n"));
	}
	void lock_row_(AsyncResponseStream *stream, lock::Lock *obj) {
	  if (obj->is_internal() && !this->include_internal_)
		return;
	  stream->print(F("esphome_lock_failed{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} 0\n"));
	  // Data itself
	  stream->print(F("esphome_lock_value{id=\""));
	  stream->print(relabel_id_(obj).c_str());
	  stream->print(F("\",name=\""));
	  stream->print(relabel_name_(obj).c_str());
	  stream->print(F("\"} "));
	  stream->print(obj->state);
	  stream->print("\n");
	}
	#endif


  web_server_base::WebServerBase *base_;
  bool include_internal_{false};
  std::map<EntityBase *, std::string> relabel_map_id_;
  std::map<EntityBase *, std::string> relabel_map_name_;
};

}  // namespace prometheus
}  // namespace esphome