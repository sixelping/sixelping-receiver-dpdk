//
// Created by rewbycraft on 2/4/20.
//
#include "application_configuration.h"

void setup_prometheus(struct app_config *aconf) {
	aconf->prom.exposer = std::make_shared<prometheus::Exposer>(aconf->prom.listen);
	aconf->prom.registry = std::make_shared<prometheus::Registry>();
	
	
	aconf->prom.exposer->RegisterCollectable(aconf->prom.registry);
	
}

void record_ping(struct app_config *aconf, std::string source_address) {
	static auto& ping_counter = prometheus::BuildCounter()
		.Name("receiver_pings_received")
		.Help("Pings received")
		.Labels({{"mac", aconf->ethdev.formatted_mac}})
		.Register(*aconf->prom.registry);
	
	ping_counter.Add({{"source_ip", source_address}}).Increment();
}