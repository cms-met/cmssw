#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"

class CandidateRefMatcher : public edm::stream::EDProducer<> {

public:
  explicit CandidateRefMatcher(const edm::ParameterSet & iConfig);
  virtual ~CandidateRefMatcher();

  virtual void produce(edm::Event & iEvent, const edm::EventSetup & iSetup) override;

private:

  edm::EDGetTokenT<edm::View<reco::Candidate>> col1Token_;            
  edm::EDGetTokenT<edm::View<reco::Candidate>> col2Token_;  

};


CandidateRefMatcher::CandidateRefMatcher(const edm::ParameterSet& iConfig) :
  col1Token_(consumes<edm::View<reco::Candidate> >( iConfig.getParameter<edm::InputTag>("col1") )),
  col2Token_(consumes<edm::View<reco::Candidate> >( iConfig.getParameter<edm::InputTag>("col2") )) {
  //register products
  
  produces<edm::PtrVector<reco::Candidate> >("col1");
  produces<edm::PtrVector<reco::Candidate> >("col2");

}


CandidateRefMatcher::~CandidateRefMatcher() {
}


// ------------ method called to produce the data  ------------
void
CandidateRefMatcher::produce(edm::Event& iEvent, const edm::EventSetup&)
{  


  std::unique_ptr<edm::PtrVector<reco::Candidate> > outcol1(new edm::PtrVector<reco::Candidate>());
  std::unique_ptr<edm::PtrVector<reco::Candidate> > outcol2(new edm::PtrVector<reco::Candidate>());

  edm::Handle< edm::View<reco::Candidate> > col1Handle;
  edm::Handle< edm::View<reco::Candidate> > col2Handle;

  iEvent.getByToken( col1Token_, col1Handle );
  iEvent.getByToken( col2Token_, col2Handle ); 

  for(size_t iC1=0;iC1<col1Handle->size();iC1++) {
    for(size_t iC2=0;iC2<col2Handle->size();iC2++) {
      if(col1Handle->ptrAt(iC1)==col2Handle->ptrAt(iC2)) {
	outcol1->push_back( col1Handle->ptrAt(iC1) );
	outcol2->push_back( col2Handle->ptrAt(iC2) );
      }
    } //col2
  } //col1

  iEvent.put(std::move(outcol1),"col1");
  iEvent.put(std::move(outcol2),"col2");
 

}

//define this as a plug-in
DEFINE_FWK_MODULE(CandidateRefMatcher);
