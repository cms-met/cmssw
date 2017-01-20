//
//

/**
*/

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/RefToBaseVector.h"
#include "DataFormats/Common/interface/PtrVector.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"


class PFCandPtrProjector : public edm::EDProducer{
  public:
    explicit PFCandPtrProjector(const edm::ParameterSet & iConfig);
    ~PFCandPtrProjector();

    virtual void produce(edm::Event & iEvent, const edm::EventSetup& iSetup) override;
    virtual void endJob() override;

  private:
    edm::EDGetTokenT<edm::View<reco::PFCandidate> > candSrcToken_;
    edm::EDGetTokenT<edm::View<reco::Candidate> > vetoSrcToken_;
};

PFCandPtrProjector::PFCandPtrProjector(const edm::ParameterSet & iConfig):
  candSrcToken_(consumes<edm::View<reco::PFCandidate> >(iConfig.getParameter<edm::InputTag>("src"))),
  vetoSrcToken_(consumes<edm::View<reco::Candidate> >(iConfig.getParameter<edm::InputTag>("veto")))
{
  produces<edm::PtrVector<reco::PFCandidate> >();
}

PFCandPtrProjector::~PFCandPtrProjector()
{
}

void
PFCandPtrProjector::produce(edm::Event & iEvent, const edm::EventSetup & iSetup)
{
  using namespace edm;
  Handle<View<reco::PFCandidate> > cands;
  iEvent.getByToken(candSrcToken_, cands);
  Handle<View<reco::Candidate> > vetos;
  iEvent.getByToken(vetoSrcToken_, vetos);

  std::unique_ptr<PtrVector<reco::PFCandidate> > result(new PtrVector<reco::PFCandidate>());
  std::set<reco::CandidatePtr> vetoedPtrs;
  for(size_t i = 0; i< vetos->size();  ++i) {
   for(size_t j=0,n=(*vetos)[i].numberOfSourceCandidatePtrs(); j<n;j++ )    {
     vetoedPtrs.insert((*vetos)[i].sourceCandidatePtr(j));   
  }
  }
 for(size_t i = 0; i< cands->size();  ++i) {
    reco::PFCandidatePtr c =  cands->ptrAt(i);
    if(vetoedPtrs.find(c)==vetoedPtrs.end())
    {
      result->push_back(c);
    }
  }
  iEvent.put(std::move(result));
}

void PFCandPtrProjector::endJob()
{
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(PFCandPtrProjector);
