#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"

class PFMatchedCandidateRefExtractor : public edm::stream::EDProducer<> {

public:
  explicit PFMatchedCandidateRefExtractor(const edm::ParameterSet & iConfig);
  virtual ~PFMatchedCandidateRefExtractor();

  virtual void produce(edm::Event & iEvent, const edm::EventSetup & iSetup) override;

private:

  edm::EDGetTokenT<edm::View<reco::Candidate>> col1Token_;            
  edm::EDGetTokenT<edm::View<reco::Candidate>> col2Token_;  
  edm::EDGetTokenT<edm::View<reco::PFCandidate> > pfCandToken_;

  bool extractPFCands_;

};


PFMatchedCandidateRefExtractor::PFMatchedCandidateRefExtractor(const edm::ParameterSet& iConfig) :
  col1Token_(consumes<edm::View<reco::Candidate> >( iConfig.getParameter<edm::InputTag>("col1") )),
  col2Token_(consumes<edm::View<reco::Candidate> >( iConfig.getParameter<edm::InputTag>("col2") ))
{
  //register products
  
  extractPFCands_=iConfig.getParameter<bool>("extractPFCandidates");

  produces<edm::PtrVector<reco::Candidate> >("col1");
  produces<edm::PtrVector<reco::Candidate> >("col2");
  if(extractPFCands_) {
    pfCandToken_=mayConsume<edm::View<reco::PFCandidate> >( iConfig.getParameter<edm::InputTag>("pfCandCollection") );
    produces<edm::PtrVector<reco::PFCandidate> >("pfCandCol1");
    produces<edm::PtrVector<reco::PFCandidate> >("pfCandCol2");
  }  

}


PFMatchedCandidateRefExtractor::~PFMatchedCandidateRefExtractor() {
}


// ------------ method called to produce the data  ------------
void
PFMatchedCandidateRefExtractor::produce(edm::Event& iEvent, const edm::EventSetup&)
{  


  std::unique_ptr<edm::PtrVector<reco::Candidate> > outcol1(new edm::PtrVector<reco::Candidate>());
  std::unique_ptr<edm::PtrVector<reco::Candidate> > outcol2(new edm::PtrVector<reco::Candidate>());
  
  std::unique_ptr<edm::PtrVector<reco::PFCandidate> > outPFCandCol1(new edm::PtrVector<reco::PFCandidate>());
  std::unique_ptr<edm::PtrVector<reco::PFCandidate> > outPFCandCol2(new edm::PtrVector<reco::PFCandidate>());

  edm::Handle< edm::View<reco::Candidate> > col1Handle;
  edm::Handle< edm::View<reco::Candidate> > col2Handle;
  edm::Handle< edm::View<reco::PFCandidate> > pfCandHandle;

  iEvent.getByToken( col1Token_, col1Handle );
  iEvent.getByToken( col2Token_, col2Handle ); 
   if(extractPFCands_) {
     iEvent.getByToken( pfCandToken_, pfCandHandle ); 
   }

  for(size_t iC1=0;iC1<col1Handle->size();iC1++) {
    for(size_t iC2=0;iC2<col2Handle->size();iC2++) {
      if(col1Handle->ptrAt(iC1)==col2Handle->ptrAt(iC2)) {
	outcol1->push_back( col1Handle->ptrAt(iC1) );
	outcol2->push_back( col2Handle->ptrAt(iC2) );

	if(!extractPFCands_) continue;
	std::set<reco::CandidatePtr> sc1s;
	std::set<reco::CandidatePtr> sc2s;
	for(size_t ics1=0;ics1<col1Handle->ptrAt(iC1)->numberOfSourceCandidatePtrs();
	    ics1++ ) {
	  sc1s.insert( col1Handle->ptrAt(iC1)->sourceCandidatePtr(ics1));
	}
	for(size_t ics2=0;ics2<col2Handle->ptrAt(iC2)->numberOfSourceCandidatePtrs();
	    ics2++ ) {
	  sc2s.insert( col1Handle->ptrAt(iC2)->sourceCandidatePtr(ics2));
	}

	for(size_t ic=0;ic<pfCandHandle->size(); ++ic) {
	  reco::PFCandidatePtr c = pfCandHandle->ptrAt(ic);
	  if(sc1s.find(c)!=sc1s.end()) {
	    outPFCandCol1->push_back(c);
	  }
	  if(sc2s.find(c)!=sc2s.end()) {
	    outPFCandCol2->push_back(c);
	  }
	} //pfcand loop
	
      } //matching
    } //col2
  } //col1

  iEvent.put(std::move(outcol1),"col1");
  iEvent.put(std::move(outcol2),"col2");
  if(extractPFCands_) {
    iEvent.put(std::move(outPFCandCol1),"pfCandCol1");
    iEvent.put(std::move(outPFCandCol2),"pfCandCol2");
  }

}

//define this as a plug-in
DEFINE_FWK_MODULE(PFMatchedCandidateRefExtractor);
