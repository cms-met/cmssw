import sys
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing
from PhysicsTools.PatAlgos.tools.tauTools import *
from RecoMET.METPUSubtraction.MVAMETConfiguration_cff import runMVAMET

options = VarParsing ('python')
options.register ('globalTag',"80X_mcRun2_asymptotic_2016_miniAODv2_v1",VarParsing.multiplicity.singleton,VarParsing.varType.string,'input global tag to be used');
options.register ('inputFile', 'file:////pnfs/desy.de/cms/tier2/store/mc/RunIISpring16MiniAODv2/DYJetsToLL_M-50_TuneCUETP8M1_13TeV-amcatnloFXFX-pythia8/MINIAODSIM/PUSpring16RAWAODSIM_reHLT_80X_mcRun2_asymptotic_v14-v1/40000/00E9D1DA-105D-E611-A56E-FA163EE988CA.root', VarParsing.multiplicity.singleton, VarParsing.varType.string, "Path to a testfile")
options.register ("localSqlite", '', VarParsing.multiplicity.singleton, VarParsing.varType.string, "Path to a local sqlite file")
options.register ("reapplyJEC", False, VarParsing.multiplicity.singleton, VarParsing.varType.bool, "Reapply JEC to Jets")
options.register ("reapplyPUJetID", False, VarParsing.multiplicity.singleton, VarParsing.varType.bool, "Reapply PU Jet ID")
options.register ("isData", False, VarParsing.multiplicity.singleton, VarParsing.varType.bool, "Input is data")
options.parseArguments()

process = cms.Process("MVAMET")

process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_cff')
jetCollection = "slimmedJets"

if (options.localSqlite == ''):
    process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff")
    process.GlobalTag.globaltag = options.globalTag
else:
    from RecoMET.METPUSubtraction.jet_recorrections import loadLocalSqlite
    loadLocalSqlite(process, options.localSqlite) 

if options.reapplyPUJetID:
    from RecoMET.METPUSubtraction.jet_recorrections import reapplyPUJetID
    reapplyPUJetID(process)

if options.reapplyJEC:
    from RecoMET.METPUSubtraction.jet_recorrections import recorrectJets
    recorrectJets(process, options.isData)
    jetCollection = "patJetsReapplyJEC"

if options.reapplyPUJetID:
    getattr(process, jetCollection).userData.userFloats.src += ['pileupJetIdUpdated:fullDiscriminant']

# configure MVA MET
runMVAMET( process, jetCollectionPF = jetCollection)

## set input files
process.source = cms.Source("PoolSource")
process.source.fileNames = cms.untracked.vstring(options.inputFile)
## logger
process.load('FWCore.MessageLogger.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 50

#! Output and Log                                                                                                                                                            
process.options   = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))
process.options.allowUnscheduled = cms.untracked.bool(True)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(options.maxEvents)
) 

process.output = cms.OutputModule("PoolOutputModule",
                                  fileName = cms.untracked.string('output.root'),
                                  outputCommands = cms.untracked.vstring(
                                                                         'keep patMETs_MVAMET_MVAMET_MVAMET',
                                                                         'keep *_patJetsReapplyJEC_*_MVAMET'
                                                                         ),        
                                  SelectEvents = cms.untracked.PSet(  SelectEvents = cms.vstring('p'))
                                  )
process.out = cms.EndPath(process.output)
