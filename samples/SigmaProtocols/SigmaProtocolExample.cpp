#include "SigmaProtocolExample.hpp"

void run_prover(std::shared_ptr<CommParty> server, SigmaDlogParams sdp, ProverVerifierExample& pe) {
	auto zp_params = make_shared<ZpGroupParams>(sdp.q, sdp.g, sdp.p);
	auto dg = make_shared<OpenSSLDlogZpSafePrime>(zp_params);
	server->join(500, 5000); // sleep time=500, timeout = 5000 (ms);
	auto g = dg->getGenerator();
	auto h = dg->exponentiate(g.get(), sdp.w);
	auto proverComputation = make_shared<SigmaDlogProverComputation>(dg, sdp.t);
	auto proverInput = make_shared<SigmaDlogProverInput>(h, sdp.w);
	pe.prove(server, proverComputation, dg, proverInput);
}

void run_verifier(shared_ptr<CommParty> server, SigmaDlogParams sdp, ProverVerifierExample& pe) {
	auto zp_params = make_shared<ZpGroupParams>(sdp.q, sdp.g, sdp.p);
	auto openSSLdg = make_shared<OpenSSLDlogZpSafePrime>(zp_params);
	auto dg = std::static_pointer_cast<DlogGroup>(openSSLdg);
	server->join(500, 5000); // sleep time=500, timeout = 5000 (ms);
	auto g = dg->getGenerator();
	auto h = dg->exponentiate(g.get(), sdp.w);
	auto commonInput = make_shared<SigmaDlogCommonInput>(h);
	auto verifierComputation = make_shared<SigmaDlogVerifierComputation>(dg, sdp.t);
	auto msg1 = make_shared<SigmaGroupElementMsg>(dg->getIdentity()->generateSendableData());
	auto msg2 = make_shared<SigmaBIMsg>();
	bool verificationPassed = pe.verify(server, verifierComputation, msg1, msg2, commonInput, openSSLdg);
	cout << "Verifer output: " << (verificationPassed ? "Success" : "Failure") << endl;

}

int mainSigma(string side, string configPath) {
	auto sdp = readSigmaConfig(configPath);
	boost::asio::io_service io_service;
	SocketPartyData proverParty(sdp.proverIp, sdp.proverPort);
	SocketPartyData verifierParty(sdp.verifierIp, sdp.verifierPort);
	shared_ptr<CommParty> server = (side == "1")?
		make_shared<CommPartyTCPSynced>(io_service, proverParty, verifierParty) :
		make_shared<CommPartyTCPSynced>(io_service, verifierParty, proverParty);
	boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
	auto pve = getProverVerifier(sdp);
	try {
		if (side == "1") {
			run_prover(server, sdp, *pve);
		}
		else if (side == "2") {
			run_verifier(server, sdp, *pve);
		}
		else {
			SigmaUsage();
			return 1;
		}
	}
	catch (const logic_error& e) {
		// Log error message in the exception object
		cerr << e.what();
	}
	io_service.stop();
	t.join();
	return 0;
}