//
// Created by rewbycraft on 1/19/20.
//

#include "grpc-helper.h"
#include "packets.h"

void setup_grpc(struct app_config *aconf) {
	RTE_LOG(INFO, APP, "Setting up GRPC...\n");
	RTE_LOG(INFO, APP, "\tCreating channel credentials...\n");
	auto creds = grpc::InsecureChannelCredentials();
	RTE_LOG(INFO, APP, "\tCreating channel...\n");
	grpc::ChannelArguments ch_args;
	ch_args.SetMaxReceiveMessageSize(64000000);
	std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(aconf->grpc.host, creds, ch_args);
	RTE_LOG(INFO, APP, "\tCreating stub...\n");
	aconf->grpc.grpc_stub = SixelpingRenderer::NewStub(channel);
	RTE_LOG(INFO, APP, "\tAttempting retrieve canvas parameters from renderer...\n");
	::google::protobuf::Empty empty;
	CanvasParametersResponse response;
	grpc::ClientContext context;
	grpc::Status status = aconf->grpc.grpc_stub->GetCanvasParameters(&context, empty, &response);
	if (!status.ok()) {
		rte_panic("GRPC failure! %d: %s\n", status.error_code(), status.error_message().c_str());
	} else {
		aconf->pixels.width = static_cast<uint16_t>(response.width());
		aconf->pixels.height = static_cast<uint16_t>(response.height());
		aconf->pixels.fps = static_cast<uint16_t>(response.fps());
		RTE_LOG(INFO, APP, "\t\tCompleted!\n");
		RTE_LOG(INFO, APP, "\tCanvas: %ux%u@%u\n", aconf->pixels.width, aconf->pixels.height, aconf->pixels.fps);
	}
	RTE_LOG(INFO, APP, "GRPC Ready!\n");
}

void send_frame_update(struct app_config *aconf, std::vector<uint8_t> image) {
	NewDeltaImageRequest request;
	std::string sImage;
	std::transform(image.begin(), image.end(), std::back_inserter(sImage), [](uint8_t c) { return char(c); });
	request.set_image(sImage);
	grpc::ClientContext context;
	::google::protobuf::Empty empty;
	grpc::Status status = aconf->grpc.grpc_stub->NewDeltaImage(&context, request, &empty);
	if (!status.ok()) {
		rte_panic("GRPC failure! %d: %s\n", status.error_code(), status.error_message().c_str());
	}
	
}

void send_metrics(struct app_config *aconf, struct eth_stats stats) {
	char mac[64];
	rte_ether_format_addr(mac, 64, &stats.addr);
	
	MetricsDatapoint dp;
	
	dp.set_ipackets(stats.ipackets);
	dp.set_opackets(stats.opackets);
	dp.set_dpackets(stats.mpackets);
	dp.set_ibytes(stats.ibytes);
	dp.set_obytes(stats.obytes);
	dp.set_mac(mac);
	
	auto& ipcounters = *dp.mutable_ipcounters();
	
	for (auto& [k,v] : aconf->pixels.senders) {
		char ip[64];
		format_ipv6_addr(ip, 64, (uint8_t*)&k);
		ipcounters[std::string(ip)] = v.load();
	}
	
	
	grpc::ClientContext context;
	::google::protobuf::Empty empty;
	
	grpc::Status status = aconf->grpc.grpc_stub->MetricsUpdate(&context, dp, &empty);
	
	if (!status.ok()) {
		rte_panic("GRPC failure! %d: %s\n", status.error_code(), status.error_message().c_str());
	}
}
