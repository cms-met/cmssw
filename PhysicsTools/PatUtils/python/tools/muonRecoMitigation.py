import FWCore.ParameterSet.Config as cms

from FWCore.GuiBrowsers.ConfigToolBase import *
#import PhysicsTools.PatAlgos.tools.helpers as configtools

def muonRecoMitigation(process,
                       pfCandCollection,
                       runOnMiniAOD,
                       selection="",
                       muonCollection="",
                       cleanCollName="cleanMuonsPFCandidates",
                       postfix=""):

    sequence=cms.Sequence()    

    if selection=="":
        if runOnMiniAOD:
            from RecoMET.METFilters.badGlobalMuonTaggersMiniAOD_cff import badGlobalMuonTagger, cloneGlobalMuonTagger
        else:
            from RecoMET.METFilters.badGlobalMuonTaggersAOD_cff import badGlobalMuonTagger, cloneGlobalMuonTagger
        setattr(process, 'badGlobalMuonTagger'+postfix, badGlobalMuonTagger.clone() )
        setattr(process, 'cloneGlobalMuonTagger'+postfix, cloneGlobalMuonTagger.clone() )
        
        badMuonCollection="badMuons"+postfix
        badMuonProducer = cms.EDProducer(
            "CandViewMerger",
            src = cms.VInputTag(
                cms.InputTag('badGlobalMuonTagger'+postfix,'bad'),
                cms.InputTag('cloneGlobalMuonTagger'+postfix,'bad'),
                )
            )
        setattr(process,badMuonCollection,badMuonProducer)
    else:
        badMuonCollection="badMuons"+postfix
        badMuonModule = cms.EDFilter("CandViewSelector", 
                                     src = cms.InputTag(muonCollection), 
                                     cut = cms.string(selection)
                                     )
    
    # noew cleaning ================================
    cleanedPFCandCollection=cleanCollName+postfix

    cleanedPFCandProducer = cms.EDProducer("CandPtrProjector", 
                                         src = cms.InputTag(pfCandCollection),
                                         veto = cms.InputTag(badMuonCollection)
                                         )

    #print "<>",cleanedPFCandCollection
    setattr(process,cleanedPFCandCollection,cleanedPFCandProducer)

    sequence +=getattr(process,"badGlobalMuonTagger")
    sequence +=getattr(process,"cloneGlobalMuonTagger")
    sequence +=getattr(process, badMuonCollection )
    sequence +=getattr(process, cleanedPFCandCollection )

    return sequence
