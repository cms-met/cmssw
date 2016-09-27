import FWCore.ParameterSet.Config as cms
import sys


import FWCore.ParameterSet.Config as cms
import sys

from RecoMET.METPUSubtraction.LeptonSelectionTools_cff import applyMuonID
from RecoMET.METPUSubtraction.LeptonSelectionTools_cff import applyElectronID
from RecoMET.METPUSubtraction.LeptonSelectionTools_cff import applyTauID
from RecoMET.METPUSubtraction.LeptonSelectionTools_cff import cleanJetsFromLeptons
from RecoMET.METProducers.PFMET_cfi import pfMet
from JetMETCorrections.Type1MET.correctionTermsPfMetType1Type2_cff import corrPfMetType1
from JetMETCorrections.Type1MET.correctedMet_cff import pfMetT1
from PhysicsTools.SelectorUtils.tools.vid_id_tools import switchOnVIDElectronIdProducer, switchOnVIDPhotonIdProducer, DataFormat, setupAllVIDIdsInModule, setupVIDElectronSelection, setupVIDPhotonSelection

def runMVAMET(process,
                 srcMuons =  "slimmedMuons", ## inputMuonCollection
                 muonTypeID    = "Tight", ## type of muon ID to be applied                                                                                                   
                 typeIsoMuons  = "dBeta", ## isolation type to be used for muons                                                                                               
                 relativeIsoCutMuons = 0.12,
                 srcElectrons = "slimmedElectrons", 
                 electronTypeID= "Tight", 
                 electronID_map = 'egmGsfElectronIDs:cutBasedElectronID-Spring15-25ns-V1-standalone-tight',
                 typeIsoElectrons = "rhoCorr",
                 relativeIsoCutEletrons = 0.12,
                 srcTaus = "slimmedTaus",
                 tauTypeID = "Loose",
                 jetCollectionPF    = "slimmedJets",
                 dRCleaning= 0.3, 
                 jetPtCut = 15, 
                 jetEtaCut =5.,
                 saveMapForTraining = False,
                 debug = False
                 ):

    if not hasattr(process,"egmGsfElectronIDs"):
        electronIdModules = ['RecoEgamma.ElectronIdentification.Identification.cutBasedElectronID_Spring15_25ns_V1_cff',
                             'RecoEgamma.ElectronIdentification.Identification.heepElectronID_HEEPV51_cff']
    
        switchOnVIDElectronIdProducer(process, DataFormat.MiniAOD)
    
        for idMod in electronIdModules:
            setupAllVIDIdsInModule(process, idMod, setupVIDElectronSelection)
    
    if not hasattr(process,"VersionedPhotonIdProducer"):
        switchOnVIDPhotonIdProducer(process, DataFormat.MiniAOD)
    
        photonIdModules = ['RecoEgamma.PhotonIdentification.Identification.cutBasedPhotonID_PHYS14_PU20bx25_V2_cff']
    
        for idMod in photonIdModules:
            setupAllVIDIdsInModule(process, idMod, setupVIDPhotonSelection)

     ## create the Path
    process.jmfw_analyzers = cms.Sequence()
    if( not hasattr(process, "p")):
        process.p = cms.Path()
    process.p += process.jmfw_analyzers
    # additional contribution from hadronically decaying taus
    from RecoMET.METPUSubtraction.tausSignificance import tausSignificance, tauMET, tauPFMET, tauDecayProducts, allDecayProducts
    process.tausSignificance = tausSignificance
    process.tauMET = tauMET
    process.tauMET.srcPFCands =  cms.InputTag("packedPFCandidates")
    process.tauPFMET = tauPFMET
    process.tauDecayProducts = tauDecayProducts
    process.allDecayProducts = allDecayProducts
   
    relativeIsoCutMuonsLoose = relativeIsoCutMuons+0.05;
    relativeIsoCutEletronsLoose = relativeIsoCutEletrons+0.05;    

    ### run Muon ID
    applyMuonID(process, 
                src   = srcMuons,
                label = muonTypeID, 
                iso_map_charged_hadron  = '',
                iso_map_neutral_hadron  = '',
                iso_map_photon          = '',
                typeIso                 = typeIsoMuons,
                relativeIsolationCutVal = relativeIsoCutMuons
                )


    ### run Electron ID
    applyElectronID(process, 
                    label = electronTypeID, 
                    src   = srcElectrons,
                    iso_map_charged_hadron  = '',
                    iso_map_neutral_hadron  = '',
                    iso_map_photon          = '',
                    typeIso = typeIsoElectrons,
                    electron_id_map = electronID_map,
                    relativeIsolationCutVal = relativeIsoCutEletrons
                    )

    ### run tau ID                                        
    applyTauID( process, 
                label = tauTypeID, 
                src = srcTaus,
                muonCollection     = srcMuons+muonTypeID,
                electronCollection = srcElectrons+electronTypeID)

    ## jet lepton cleaning

    cleanJetsFromLeptons(process,
                         label = "Cleaned",
                         jetCollection      = jetCollectionPF,
                         muonCollection     = srcMuons+muonTypeID,
                         electronCollection = srcElectrons+electronTypeID,
                         tauCollection      = srcTaus+tauTypeID+"Cleaned",
                         jetPtCut   = jetPtCut,
                         jetEtaCut  = jetEtaCut,
                         dRCleaning = dRCleaning)


    #### Input definitions like in classic MVA MET
    #### tracks from PV
    process.pfCHS = cms.EDFilter("CandPtrSelector",
                                       cut = cms.string('fromPV'),
                                       src = cms.InputTag("packedPFCandidates")
                                       )
    process.pfChargedPV = cms.EDFilter("CandPtrSelector",
                                       cut = cms.string('fromPV && charge !=0'),
                                       src = cms.InputTag("packedPFCandidates")
                                       )
    #### tracks not from PV
    process.pfChargedPU = cms.EDFilter("CandPtrSelector",
                                       cut = cms.string('!fromPV && charge !=0'),
                                       src = cms.InputTag("packedPFCandidates")
                                       )
    process.pfNeutrals  = cms.EDFilter("CandPtrSelector",
                                       cut = cms.string('charge ==0'),
                                       src = cms.InputTag("packedPFCandidates")
                                       )
    #### Neutrals in Jets passing PU Jet ID
    #### and Neutrals in Jets not passing PU Jet ID
    process.neutralInJets = cms.EDProducer("neutralCandidatePUIDJets",
                                           srcJets = cms.InputTag(jetCollectionPF+"Cleaned"),
                                           srcCandidates = cms.InputTag("pfNeutrals"),
                                           neutralParticlesPVJetsLabel = cms.string("neutralPassingPUIDJets"),
                                           neutralParticlesPUJetsLabel = cms.string("neutralFailingPUIDJets"),
                                           neutralParticlesUnclusteredLabel = cms.string("neutralParticlesUnclustered"),
                                           jetPUDIWP = cms.string("user"),
                                           jetPUIDMapLabel = cms.string("fullDiscriminant"))
  

    #### Merge collections to produce corresponding METs
    process.pfMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(          
                                                                                          cms.InputTag("pfChargedPV"),
                                                                                          cms.InputTag("pfChargedPU"),
                                                                                          cms.InputTag("neutralInJets", "neutralPassingPUIDJets"),
                                                                                          cms.InputTag("neutralInJets", "neutralFailingPUIDJets"),
                                                                                          cms.InputTag("neutralInJets", "neutralParticlesUnclustered")
    ))
    #### Track MET
    process.pfTrackMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(cms.InputTag("pfChargedPV")))
    ## No-PU MET
    process.pfNoPUMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(        cms.InputTag("pfChargedPV"),
                                                                                          cms.InputTag("neutralInJets", "neutralPassingPUIDJets")))
    ## PU corrected MET
    process.pfPUCorrectedMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag( cms.InputTag("pfChargedPV"), 
                                                                                          cms.InputTag("neutralInJets", "neutralPassingPUIDJets"),
                                                                                          cms.InputTag("neutralInJets", "neutralParticlesUnclustered")))
    ## PU MET
    process.pfPUMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(          cms.InputTag("pfChargedPU"),
                                                                                          cms.InputTag("neutralInJets", "neutralFailingPUIDJets")))
    
    ##Experimental
    process.pfChargedPUMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(cms.InputTag("pfChargedPU")))
    process.pfNeutralPUMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(cms.InputTag("neutralInJets", "neutralFailingPUIDJets")))
    process.pfNeutralPVMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(cms.InputTag("neutralInJets", "neutralPassingPUIDJets")))
    process.pfNeutralUnclusteredMETCands = cms.EDProducer("CandViewMerger", src = cms.VInputTag(cms.InputTag("neutralInJets", "neutralParticlesUnclustered")))

                                                          
    from PhysicsTools.PatAlgos.producersLayer1.metProducer_cfi import patMETs
    patMETsForMVA = patMETs.clone()
    patMETsForMVA.computeMETSignificance = cms.bool(False)
    patMETsForMVA.addGenMET = cms.bool(False)
    patMETsForMVA.srcJets = cms.InputTag(jetCollectionPF)

    setattr(patMETsForMVA,"srcLeptons", cms.VInputTag(srcMuons+muonTypeID,srcElectrons+electronTypeID,srcTaus+tauTypeID+"Cleaned"))

    # get jets for T1 Correction
    from RecoJets.JetProducers.ak4PFJets_cfi import ak4PFJets
    from JetMETCorrections.Configuration.JetCorrectors_cff import ak4PFCHSL1FastL2L3Corrector,ak4PFCHSL3AbsoluteCorrector,ak4PFCHSL2RelativeCorrector,ak4PFCHSL1FastjetCorrector, ak4PFCHSL1FastL2L3ResidualCorrector, ak4PFCHSResidualCorrector
    process.ak4PFCHSL1FastL2L3Corrector = ak4PFCHSL1FastL2L3Corrector
    process.ak4PFCHSL1FastL2L3ResidualCorrector = ak4PFCHSL1FastL2L3ResidualCorrector
    process.ak4PFCHSResidualCorrector = ak4PFCHSResidualCorrector
    process.ak4PFCHSL3AbsoluteCorrector = ak4PFCHSL3AbsoluteCorrector
    process.ak4PFCHSL2RelativeCorrector = ak4PFCHSL2RelativeCorrector
    process.ak4PFCHSL1FastjetCorrector = ak4PFCHSL1FastjetCorrector 


    for met in ["pfMET", "pfTrackMET", "pfNoPUMET", "pfPUCorrectedMET", "pfPUMET", "pfChargedPUMET", "pfNeutralPUMET", "pfNeutralPVMET", "pfNeutralUnclusteredMET"]:
        # create PF METs
        setattr(process, met, pfMet.clone(src = cms.InputTag(met+"Cands"), alias = cms.string(met)))
        # create Jets
        setattr(process, "ak4JetsFor"+met, ak4PFJets.clone(src = cms.InputTag(met+"Cands")))
        setattr(process, "corr"+met, corrPfMetType1.clone(src = cms.InputTag("ak4JetsFor"+met)))
        # derive corrections and apply them
        setattr(process, met+"T1", pfMetT1.clone( src= cms.InputTag(met), srcCorrections=cms.VInputTag(cms.InputTag("corr"+met, "type1"))))
        # convert METs to pat objects
        setattr(process, "pat"+met,      patMETsForMVA.clone(metSource = cms.InputTag(met)))
        setattr(process, "pat"+met+"T1", patMETsForMVA.clone(metSource = cms.InputTag(met+"T1")))

    process.pfChs = cms.EDProducer("CandViewMerger", src = cms.VInputTag(          
                                                                                          cms.InputTag("pfChargedPV"),
                                                                                          cms.InputTag("neutralInJets", "neutralPassingPUIDJets"),
                                                                                          cms.InputTag("neutralInJets", "neutralFailingPUIDJets"),
                                                                                          cms.InputTag("neutralInJets", "neutralParticlesUnclustered")
    ))
    process.ak4JetsForpfMET.src = cms.InputTag("pfChs")

    ### MVA MET
    process.MVAMET = cms.EDProducer("MVAMET",                                                
                                    debug = cms.bool(debug),
                                    requireOS = cms.bool(True),
                                    combineNLeptons = cms.int32(2),
                                    MVAMETLabel = cms.string("MVAMET"),
                                    srcMETs      = cms.VInputTag(
                                                                 cms.InputTag("slimmedMETs"),
                                                                 cms.InputTag("patpfMET"),
                                                                 cms.InputTag("patpfMETT1"),
                                                                 cms.InputTag("patpfTrackMET"),
                                                                 #cms.InputTag("patpfTrackMETT1"),
                                                                 cms.InputTag("patpfNoPUMET"),
                                                                 #cms.InputTag("patpfNoPUMETT1"),
                                                                 cms.InputTag("patpfPUCorrectedMET"),
                                                                 #cms.InputTag("patpfPUCorrectedMETT1"),
                                                                 cms.InputTag("patpfPUMET"),
                                                                 #cms.InputTag("patpfPUMETT1"),
                                                                 cms.InputTag("slimmedMETsPuppi"),
                                                                 #cms.InputTag("patpfChargedPUMET"),
                                                                 #cms.InputTag("patpfNeutralPUMET"),
                                                                 #cms.InputTag("patpfNeutralPVMET"),
                                                                 #cms.InputTag("patpfNeutralUnclusteredMET")
                                                                ),
                                    #inputMETFlags = cms.vint32(0,0,0,1,1,0,0,0,0,3,3,0,3,3,2,3), # use this flags if all MET collections are used above
                                    inputMETFlags = cms.vint32(0,0,0,1,0,0,3,0),
                                    srcJets        = cms.InputTag(jetCollectionPF+"Cleaned"),
                                    srcVertices    = cms.InputTag("offlineSlimmedPrimaryVertices"),
                                    srcTaus        = cms.InputTag(srcTaus+tauTypeID+"Cleaned"),
                                    srcMuons       = cms.InputTag(srcMuons+muonTypeID),
                                    weightFile     = cms.FileInPath('RecoMET/METPUSubtraction/data/weightfile.root'),
                                    #srcLeptons  = cms.VInputTag("slimmedMuons", "slimmedElectrons", "slimmedTaus"), # to produce all possible combinations
                                    srcLeptons  = cms.VInputTag(srcMuons+muonTypeID,srcElectrons+electronTypeID,srcTaus+tauTypeID+"Cleaned"), # to produce a selection specifically designed for trainings
                                    useTauSig = cms.bool(True),
                                    tausSignificance = cms.InputTag('tausSignificance', 'METCovariance'),
                                    saveMap = cms.bool(saveMapForTraining),
                                    permuteLeptonsWithinPlugin = cms.bool(True)
                                    )
