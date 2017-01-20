#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"

class JetCollectionReducer : public edm::stream::EDProducer<> {

public:
  explicit JetCollectionReducer(const edm::ParameterSet & iConfig);
  virtual ~JetCollectionReducer();

  virtual void produce(edm::Event & iEvent, const edm::EventSetup & iSetup) override;

private:

  edm::EDGetTokenT<std::vector<pat::Jet>> jetColToken_;            

  std::vector<edm::EDGetTokenT<edm::View<reco::Candidate> > > collections_;
  
};


JetCollectionReducer::JetCollectionReducer(const edm::ParameterSet& iConfig) :
  jetColToken_(consumes<std::vector<pat::Jet> >( iConfig.getParameter<edm::InputTag>("jetCollection") ))
{
  
  std::vector<edm::InputTag> filtersDecTags = iConfig.getParameter<std::vector<edm::InputTag> >("triggeringCollections");
  for(std::vector<edm::InputTag>::const_iterator inputTag=filtersDecTags.begin();
      inputTag!=filtersDecTags.end();++inputTag) {
    collections_.push_back(consumes<edm::View<reco::Candidate> >(*inputTag));
  }
  
  produces<std::vector<pat::Jet> >();

}


JetCollectionReducer::~JetCollectionReducer() {
}


// ------------ method called to produce the data  ------------
void
JetCollectionReducer::produce(edm::Event& iEvent, const edm::EventSetup&)
{  

  std::unique_ptr<std::vector<pat::Jet> > outJets(new std::vector<pat::Jet>());
  
  bool filterDecision=false;
  edm::Handle<edm::View<reco::Candidate> > tmpCol;
  for(std::vector<edm::EDGetTokenT<edm::View<reco::Candidate> > >::const_iterator filter=collections_.begin();
      filter!=collections_.end();filter++) {
    iEvent.getByToken(*filter,tmpCol);
    if(tmpCol->size()!=0) {
      filterDecision=true;
      break;
    }
  }

  if(!filterDecision) {
    iEvent.put(std::move(outJets));
    return;
  }
  
  edm::Handle< std::vector<pat::Jet> > jetColHandle;
  iEvent.getByToken( jetColToken_, jetColHandle );
  
  //MM: probably a better way to do it...
  for(size_t ij=0;ij<jetColHandle->size();ij++) {
    outJets->push_back( (*jetColHandle)[ij] );
  }

  iEvent.put(std::move(outJets));

}

//define this as a plug-in
DEFINE_FWK_MODULE(JetCollectionReducer);
