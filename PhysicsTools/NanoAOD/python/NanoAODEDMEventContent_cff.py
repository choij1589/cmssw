import FWCore.ParameterSet.Config as cms

NanoAODEDMEventContent = cms.PSet(
    outputCommands = cms.untracked.vstring(
        'drop *',
        "keep nanoaodFlatTable_*Table_*_*",     # event data
        "keep edmTriggerResults_*_*_*",  # event data
        "keep String_*_genModel_*",  # generator model data
        "keep nanoaodMergeableCounterTable_*Table_*_*", # accumulated per/run or per/lumi data
        "keep nanoaodUniqueString_nanoMetadata_*_*",   # basic metadata
        #"drop Tau_*",  # drop tau collection
        #"drop nanoaodFlatTable_Tau*_*_*",  # drop tau table
        #"drop *_tauTable_*_*",  # drop tau table
        #"drop *_tausMCMatchLepTauForTable_*_*",  # drop taus MC match lep tau for table
        #"drop *_tausMCMatchHadTauForTable_*_*",  # drop taus MC match had tau for table
        #"drop *_tauMCTable_*_*",  # drop tau MC table
        #"drop *_boostedTauTable_*_*",  # drop boosted tau table
        #"drop *_boostedTauMCTable_*_*",  # drop boosted tau MC table
        #"drop *_fatJetTable_*_*",  # drop fatjet table
        #"drop *_fatJetMCTable_*_*",  # drop fatjet MC table
        #"drop *_saJetTable_*_*",  # drop soft activity jet table
        #"drop *_saTable_*_*",  # drop soft activity table
        #"drop *_subJetTable_*_*",  # drop subjet table
        #"drop *_subjetMCTable_*_*",  # drop subjet MC table
        #"drop *_genJetAK8Table_*_*",  # drop genjet AK8 table
        #"drop *_genJetAK8FlavourAssociation_*_*",  # drop genjet AK8 flavour association
        #"drop *_genJetAK8FlavourTable_*_*",  # drop genjet AK8 flavour table
        #"drop *_fatJetMCTable_*_*",  # drop fatjet MC table
        #"drop *_genSubJetAK8Table_*_*",  # drop gen subjet AK8 table
        #"drop *_lowPtElectronTable_*_*",  # drop low pt electron table
        #"drop *_lowPtElectronMCTable_*_*",  # drop low pt electron MC table
        #"drop *_photonTable_*_*",  # drop photon table
        #"drop *_photonMCTable_*_*",  # drop photon MC table
        #"drop *_photonsMCMatchForTable_*_*",  # drop photons MC match for table
        #"drop *_caloMetTable_*_*",  # drop calo met table
        #"drop *_chsMetTable_*_*",  # drop chs met table
    )
)

NANOAODEventContent = NanoAODEDMEventContent.clone(
    compressionLevel = cms.untracked.int32(9),
    compressionAlgorithm = cms.untracked.string("LZMA"),
)
NANOAODSIMEventContent = NanoAODEDMEventContent.clone(
    compressionLevel = cms.untracked.int32(9),
    compressionAlgorithm = cms.untracked.string("LZMA"),
)

NanoGenOutput = NanoAODEDMEventContent.outputCommands[:]
NanoGenOutput.remove("keep edmTriggerResults_*_*_*")

NANOAODGENEventContent = cms.PSet(
    compressionLevel = cms.untracked.int32(9),
    compressionAlgorithm = cms.untracked.string("LZMA"),
    outputCommands = cms.untracked.vstring(NanoGenOutput)
)
