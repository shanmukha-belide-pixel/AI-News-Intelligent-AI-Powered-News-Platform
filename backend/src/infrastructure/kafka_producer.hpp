#pragma once
// =============================================================================
// kafka_producer.hpp  —  Event-driven telemetry producer wrapping librdkafka
// =============================================================================
#include <string>
#include <memory>
#include <librdkafka/rdkafkacpp.h>
#include <spdlog/spdlog.h>
#include "../config/app_config.hpp"

namespace news {

class KafkaProducer {
public:
    static KafkaProducer* instance() {
        static KafkaProducer inst;
        return &inst;
    }

    // ── Produce message to a topic ───────────────────────────────────────────
    void send(const std::string& topic_name,
              const std::string& key,
              const std::string& payload) noexcept
    {
        if (!producer_) {
            spdlog::warn("[Kafka] Producer not connected. Discarding message: {}", payload);
            return;
        }

        // Create topic configuration if needed
        std::string errstr;
        
        // Produce message
        RdKafka::ErrorCode resp = producer_->produce(
            topic_name,
            RdKafka::Topic::PARTITION_UA, // Auto partition assignment
            RdKafka::Producer::RK_MSG_COPY, // Copy payload
            const_cast<char*>(payload.c_str()), payload.size(),
            key.empty() ? nullptr : const_cast<char*>(key.c_str()), key.size(),
            0, // Timestamp
            nullptr, // Message private data
            nullptr // Headers
        );

        if (resp != RdKafka::ERR_NO_ERROR) {
            spdlog::error("[Kafka] Failed to produce message to {}: {}", topic_name, RdKafka::err2str(resp));
        } else {
            spdlog::debug("[Kafka] Message produced to {} [key: {}]", topic_name, key);
        }

        // Poll to trigger delivery reports
        producer_->poll(0);
    }

    bool is_connected() const { return producer_ != nullptr; }

private:
    // Custom Delivery Report Callback
    class DeliveryReportCb : public RdKafka::DeliveryReportCb {
    public:
        void dr_cb(RdKafka::Message &message) override {
            if (message.err()) {
                spdlog::error("[Kafka] Message delivery failed: {}", message.errstr());
            } else {
                spdlog::debug("[Kafka] Message delivered successfully to partition {}", message.partition());
            }
        }
    };

    KafkaProducer() {
        const auto& cfg = get_config().kafka;
        
        std::string errstr;
        auto conf = std::unique_ptr<RdKafka::Conf>(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
        
        if (conf->set("bootstrap.servers", cfg.brokers, errstr) != RdKafka::Conf::CONF_OK) {
            spdlog::error("[Kafka] Config error (bootstrap.servers): {}", errstr);
            return;
        }
        
        if (conf->set("client.id", cfg.client_id, errstr) != RdKafka::Conf::CONF_OK) {
            spdlog::error("[Kafka] Config error (client.id): {}", errstr);
            return;
        }

        // Set delivery callback
        dr_cb_ = std::make_unique<DeliveryReportCb>();
        if (conf->set("dr_cb", dr_cb_.get(), errstr) != RdKafka::Conf::CONF_OK) {
            spdlog::error("[Kafka] Config error (dr_cb): {}", errstr);
            return;
        }

        // Create producer instance
        producer_ = std::unique_ptr<RdKafka::Producer>(RdKafka::Producer::create(conf.get(), errstr));
        if (!producer_) {
            spdlog::error("[Kafka] Failed to create producer: {}", errstr);
        } else {
            spdlog::info("[Kafka] Producer initialized with brokers: {}", cfg.brokers);
        }
    }

    std::unique_ptr<RdKafka::Producer> producer_;
    std::unique_ptr<DeliveryReportCb> dr_cb_;
};

} // namespace news
