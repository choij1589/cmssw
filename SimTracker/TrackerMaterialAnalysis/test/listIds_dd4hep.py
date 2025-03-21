#! /usr/bin/env cmsRun

import sys
import FWCore.ParameterSet.Config as cms

from Configuration.Geometry.GeometryDD4hepExtended2021_cff import *

process = cms.Process("MaterialAnalyser")
process.load('FWCore.MessageService.MessageLogger_cfi')


if hasattr(process,'MessageLogger'):
    process.MessageLogger.ListIds=dict()

process.source = cms.Source("EmptySource")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)

process.DDDetectorESProducer = cms.ESSource("DDDetectorESProducer",
    confGeomXMLFiles = cms.FileInPath(
        'Geometry/TrackerCommonData/data/dd4hep/cms-tracker-geometry-2021.xml'
    ),
    appendToDataLabel = cms.string('CMS')
)


process.DDSpecParRegistryESProducer = cms.ESProducer(
    "DDSpecParRegistryESProducer"
)

process.DDCompactViewESProducer = cms.ESProducer(
    "DDCompactViewESProducer",
    appendToDataLabel = cms.string('CMS')
)

process.test = cms.EDAnalyzer(
    "DD4hep_ListIds",
    materials = cms.untracked.vstring("materials:Silicon" , "tracker:SenSi", "materials:Air"),
    printMaterial = cms.untracked.bool(False)
)

process.path = cms.Path(
    process.test
)
