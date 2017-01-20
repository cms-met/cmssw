import FWCore.ParameterSet.Config as cms

from CommonTools.ParticleFlow.pfNoPileUpJME_cff import *

def makeAlternateSlimmedJetCollection(process,
                                      pfCandCollection,
                                      #labelName=
                                      CHS=True,
                                      postfix=""):

 
    pfCandColl=pfCandCollection

    if not hasattr(process, "ak4PFJets"+postfix):
        from RecoJets.JetProducers.ak4PFJets_cfi import ak4PFJets
        from RecoJets.JetAssociationProducers.j2tParametersVX_cfi import j2tParametersVX
        import PhysicsTools.PatAlgos.tools.helpers as configtools
     

        setattr( process, "tmpPFCandCollPtr"+postfix,
                 cms.EDProducer("PFCandidateFwdPtrProducer",
                                src = pfCandCollection ) )

        configtools.cloneProcessingSnippet(process, 
                                           getattr(process,"pfNoPileUpJMESequence"),
                                           postfix )
        getattr(process, "pfPileUpJME"+postfix).PFCandidates = cms.InputTag("tmpPFCandCollPtr"+postfix)
        pfCHS = getattr(process, "pfNoPileUpJME").clone( bottomCollection = cms.InputTag("tmpPFCandCollPtr"+postfix) )
        if not hasattr(process, "pfCHS"+postfix):
            setattr(process,"pfCHS"+postfix,pfCHS)
        pfCandColl = cms.InputTag("pfCHS"+postfix)
            
        setattr(process, "ak4PFJets"+postfix, ak4PFJets.clone(
                doAreaFastjet = True,
                src=cms.InputTag(pfCandColl) #"cleanMuonsPFCandidates"
                ) )
     

    setattr(process, "ak4PFJetsTracksAssociatorAtVertex"+postfix,
            cms.EDProducer("JetTracksAssociatorAtVertex",
                           j2tParametersVX,
                           jets = cms.InputTag("ak4PFJets"+postfix)
                           ))
    setattr(process, "patJetCharge"+postfix,
            cms.EDProducer("JetChargeProducer",
            src = cms.InputTag("ak4PFJetsPuppiTracksAssociatorAtVertex"+postfix),
            var = cms.string('Pt'),
            exp = cms.double(1.0)
                               ) )
    configtools.cloneProcessingSnippet(process, 
                                       getattr(process,"makePatJets"),
                                       postfix) 
        
    addJetCollection(process, postfix="", labelName = postfix,
                     jetSource = cms.InputTag("ak4PFJets"+postfix),
                     jetCorrections = ('AK4PF', ['L2Relative', 'L3Absolute'], ''),
                     algo= 'AK', rParam = 0.4,
                     btagDiscriminators = map(lambda x: x.value() ,
                                              process.patJets.discriminatorSources)
                     )
