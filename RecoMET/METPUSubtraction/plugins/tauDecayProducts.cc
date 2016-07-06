

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
 
//#include "FWCore/ParameterSet/interface/ParameterSet.h"
//#include "FWCore/MessageLogger/interface/MessageLogger.h"
//#include "FWCore/ServiceRegistry/interface/Service.h"
 
//#include "DataFormats/Common/interface/Handle.h"
//#include "DataFormats/Candidate/interface/Candidate.h"
//#include "DataFormats/Candidate/interface/CandidateFwd.h"
//#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "DataFormats/PatCandidates/interface/Tau.h"
#include <iostream>
#include <memory>

using namespace std;
using namespace edm;
using namespace reco;


////////////////////////////////////////////////////////////////////////////////
// class definition
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
class tauDecayProducts : public edm::EDProducer
{
public:
  // construction/destruction
  tauDecayProducts(const edm::ParameterSet& iConfig);
  ~tauDecayProducts() {;}
  
  // member functions
  void produce(edm::Event& iEvent,const edm::EventSetup& iSetup);
  void endJob();

private:
  // member data
  edm::EDGetTokenT<pat::TauCollection> src_;

  std::string  moduleName_;

};




////////////////////////////////////////////////////////////////////////////////
// construction/destruction
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
tauDecayProducts::tauDecayProducts(const edm::ParameterSet& iConfig)
  : moduleName_(iConfig.getParameter<string>("@module_label"))
{
  src_ = consumes<pat::TauCollection>(iConfig.getParameter<edm::InputTag>("src"));
  produces<reco::PFCandidateCollection>();
}


////////////////////////////////////////////////////////////////////////////////
// implementation of member functions
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
void tauDecayProducts::produce(edm::Event& iEvent,const edm::EventSetup& iSetup)
{
  edm::Handle<pat::TauCollection> tauCollectionHandle;
  iEvent.getByToken(src_, tauCollectionHandle);
  const pat::TauCollection tauCollection = *(tauCollectionHandle.product());
  
  
  
  auto_ptr<reco::PFCandidateCollection> recoCands(new vector<reco::PFCandidate>);

  for (auto it : tauCollection)
  {
    for(auto cand : it.signalPFChargedHadrCands() )
    {
      recoCands->push_back(cand);
    }
    for(auto cand : it.signalPFGammaCands() )
    {
      recoCands->push_back(cand);
    }
  }
  
  //iEvent.put(recoCands,"convertedPackedPFCandidates");
  iEvent.put(recoCands);
}


//______________________________________________________________________________
void tauDecayProducts::endJob()
{
}


////////////////////////////////////////////////////////////////////////////////
// plugin definition
////////////////////////////////////////////////////////////////////////////////

DEFINE_FWK_MODULE(tauDecayProducts);
