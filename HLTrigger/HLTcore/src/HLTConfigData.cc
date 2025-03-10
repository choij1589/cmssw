/** \class HLTConfigData
 *
 * See header file for documentation
 *
 *
 *  \author Martin Grunewald
 *
 */

#include "HLTrigger/HLTcore/interface/HLTConfigData.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/path_configuration.h"
#include "FWCore/Utilities/interface/thread_safety_macros.h"
#include "FWCore/Utilities/interface/transform.h"

#include <iostream>

//Using this function with the 'const static within s_dummyPSet'
// guarantees that even if multiple threads call s_dummyPSet at the
// same time, only the 'first' one registers the dummy PSet.
static const edm::ParameterSet initializeDummyPSet() {
  edm::ParameterSet dummy;
  dummy.registerIt();
  return dummy;
}

static const edm::ParameterSet* s_dummyPSet() {
  static const edm::ParameterSet dummyPSet{initializeDummyPSet()};
  return &dummyPSet;
}

HLTConfigData::HLTConfigData()
    : processPSet_(s_dummyPSet()),
      processName_(""),
      globalTag_(""),
      tableName_(),
      triggerNames_(),
      moduleLabels_(),
      saveTagsModules_(),
      triggerIndex_(),
      moduleIndex_(),
      l1tType_(0),
      hltL1GTSeeds_(),
      hltL1TSeeds_(),
      streamNames_(),
      streamIndex_(),
      streamContents_(),
      datasetNames_(),
      datasetIndex_(),
      datasetContents_(),
      hltPrescaleTable_(),
      hltPrescaleTableValuesDouble_{},
      hltPrescaleTableValuesFractional_{} {
  if (processPSet_->id().isValid()) {
    extract();
  }
}

HLTConfigData::HLTConfigData(const edm::ParameterSet* iPSet)
    : processPSet_(iPSet),
      processName_(""),
      globalTag_(""),
      tableName_(),
      triggerNames_(),
      moduleLabels_(),
      saveTagsModules_(),
      triggerIndex_(),
      moduleIndex_(),
      l1tType_(0),
      hltL1GTSeeds_(),
      hltL1TSeeds_(),
      streamNames_(),
      streamIndex_(),
      streamContents_(),
      datasetNames_(),
      datasetIndex_(),
      datasetContents_(),
      hltPrescaleTable_(),
      hltPrescaleTableValuesDouble_{},
      hltPrescaleTableValuesFractional_{} {
  if (processPSet_->id().isValid()) {
    extract();
  }
}

void HLTConfigData::extract() {
  using namespace std;
  using namespace edm;
  using namespace trigger;

  // Extract process name
  CMS_SA_ALLOW if (processPSet_->existsAs<string>("@process_name", true)) {
    processName_ = processPSet_->getParameter<string>("@process_name");
  }

  // Extract globaltag
  globalTag_ = "";
  const ParameterSet* GlobalTagPSet(nullptr);
  CMS_SA_ALLOW if (processPSet_->exists("GlobalTag")) { GlobalTagPSet = &(processPSet_->getParameterSet("GlobalTag")); }
  else if (processPSet_->exists("PoolDBESSource@GlobalTag")) {
    GlobalTagPSet = &(processPSet_->getParameterSet("PoolDBESSource@GlobalTag"));
  }
  CMS_SA_ALLOW if (GlobalTagPSet && GlobalTagPSet->existsAs<std::string>("globaltag", true)) {
    globalTag_ = GlobalTagPSet->getParameter<std::string>("globaltag");
  }

  // Obtain PSet containing table name (available only in 2_1_10++ files)
  CMS_SA_ALLOW if (processPSet_->existsAs<ParameterSet>("HLTConfigVersion", true)) {
    const ParameterSet& HLTPSet(processPSet_->getParameterSet("HLTConfigVersion"));
    CMS_SA_ALLOW if (HLTPSet.existsAs<string>("tableName", true)) {
      tableName_ = HLTPSet.getParameter<string>("tableName");
    }
  }

  // Extract trigger paths (= paths - end_paths)
  CMS_SA_ALLOW if (processPSet_->existsAs<ParameterSet>("@trigger_paths", true)) {
    const ParameterSet& HLTPSet(processPSet_->getParameterSet("@trigger_paths"));
    CMS_SA_ALLOW if (HLTPSet.existsAs<vector<string>>("@trigger_paths", true)) {
      triggerNames_ = HLTPSet.getParameter<vector<string>>("@trigger_paths");
    }
  }

  // Obtain module labels of all modules on all trigger paths
  const unsigned int n(size());
  moduleLabels_.reserve(n);
  for (unsigned int i = 0; i != n; ++i) {
    CMS_SA_ALLOW if (processPSet_->existsAs<vector<string>>(triggerNames_[i], true)) {
      moduleLabels_.push_back(path_configuration::configurationToModuleBitPosition(
          processPSet_->getParameter<vector<string>>(triggerNames_[i])));
    }
  }
  saveTagsModules_.reserve(n);
  vector<string> labels;
  for (unsigned int i = 0; i != n; ++i) {
    labels.clear();
    const vector<string>& modules(moduleLabels(i));
    const unsigned int m(modules.size());
    labels.reserve(m);
    for (unsigned int j = 0; j != m; ++j) {
      const string& label(modules[j]);
      if (saveTags(label))
        labels.push_back(label);
    }
    saveTagsModules_.push_back(labels);
  }

  // Fill index maps for fast lookup
  moduleIndex_.resize(n);
  for (unsigned int i = 0; i != n; ++i) {
    triggerIndex_[triggerNames_[i]] = i;
    moduleIndex_[i].clear();
    const unsigned int m(size(i));
    for (unsigned int j = 0; j != m; ++j) {
      moduleIndex_[i][moduleLabels_[i][j]] = j;
    }
  }

  // Extract and fill HLTLevel1GTSeed information for each trigger path
  hltL1GTSeeds_.resize(n);
  for (unsigned int i = 0; i != n; ++i) {
    hltL1GTSeeds_[i].clear();
    const unsigned int m(size(i));
    for (unsigned int j = 0; j != m; ++j) {
      const string& label(moduleLabels_[i][j]);
      //HLTConfigProvider sees ignored modules as "-modname"
      //if the HLTLevel1GTSeed is ignored in the config, it shouldnt
      //count to the number of active HLTLevel1GTSeeds so we now check
      //for the module being ignored
      if (label.front() != '-' && moduleType(label) == "HLTLevel1GTSeed") {
        const ParameterSet& pset(modulePSet(label));
        if (pset != ParameterSet()) {
          const bool l1Tech(pset.getParameter<bool>("L1TechTriggerSeeding"));
          const string l1Seed(pset.getParameter<string>("L1SeedsLogicalExpression"));
          hltL1GTSeeds_[i].push_back(pair<bool, string>(l1Tech, l1Seed));
        }
      }
    }
  }

  // Extract and fill HLTL1TSeed information for each trigger path
  hltL1TSeeds_.resize(n);
  for (unsigned int i = 0; i != n; ++i) {
    hltL1TSeeds_[i].clear();
    const unsigned int m(size(i));
    for (unsigned int j = 0; j != m; ++j) {
      const string& label(moduleLabels_[i][j]);
      //HLTConfigProvider sees ignored modules as "-modname"
      //if the HLTL1TSeed is ignored in the config, it shouldnt
      //count to the number of active HLTL1TSeeds so we now check
      //for the module being ignored
      if (label.front() != '-' && moduleType(label) == "HLTL1TSeed") {
        const ParameterSet& pset(modulePSet(label));
        if (pset != ParameterSet()) {
          const string l1Gtag(pset.getParameter<edm::InputTag>("L1GlobalInputTag").label());
          // Emulator output is used to ignore L1T prescales
          if (l1Gtag != "hltGtStage2ObjectMap") {
            const string l1Seed(pset.getParameter<string>("L1SeedsLogicalExpression"));
            hltL1TSeeds_[i].push_back(l1Seed);
          }
        }
      }
    }
  }

  // Extract and fill streams information
  CMS_SA_ALLOW if (processPSet_->existsAs<ParameterSet>("streams", true)) {
    const ParameterSet& streams(processPSet_->getParameterSet("streams"));
    streamNames_ = streams.getParameterNamesForType<vector<string>>();
    sort(streamNames_.begin(), streamNames_.end());
    const unsigned int n(streamNames_.size());
    streamContents_.resize(n);
    for (unsigned int i = 0; i != n; ++i) {
      streamIndex_[streamNames_[i]] = i;
      streamContents_[i] = streams.getParameter<vector<string>>(streamNames_[i]);
      sort(streamContents_[i].begin(), streamContents_[i].end());
    }
  }

  // Extract and fill datasets information
  CMS_SA_ALLOW if (processPSet_->existsAs<ParameterSet>("datasets", true)) {
    const ParameterSet& datasets(processPSet_->getParameterSet("datasets"));
    datasetNames_ = datasets.getParameterNamesForType<vector<string>>();
    sort(datasetNames_.begin(), datasetNames_.end());
    const unsigned int n(datasetNames_.size());
    datasetContents_.resize(n);
    for (unsigned int i = 0; i != n; ++i) {
      datasetIndex_[datasetNames_[i]] = i;
      datasetContents_[i] = datasets.getParameter<vector<string>>(datasetNames_[i]);
      sort(datasetContents_[i].begin(), datasetContents_[i].end());
    }
  }

  // Extract and fill Prescale information

  // Check various possibilities to get the HLT prescale sets:
  string prescaleName("");
  const string preS("PrescaleService");
  const string preT("PrescaleTable");
  CMS_SA_ALLOW if (processPSet_->existsAs<ParameterSet>(preS, true)) { prescaleName = preS; }
  else if (processPSet_->existsAs<ParameterSet>(preT, true)) {
    prescaleName = preT;
  }
  if (prescaleName.empty()) {
    hltPrescaleTable_ = HLTPrescaleTable();
  } else {
    const ParameterSet& iPS(processPSet_->getParameterSet(prescaleName));
    string defaultLabel("default");
    CMS_SA_ALLOW if (iPS.existsAs<string>("lvl1DefaultLabel", true)) {
      defaultLabel = iPS.getParameter<string>("lvl1DefaultLabel");
    }
    vector<string> labels;
    CMS_SA_ALLOW if (iPS.existsAs<vector<string>>("lvl1Labels", true)) {
      labels = iPS.getParameter<vector<string>>("lvl1Labels");
    }
    unsigned int set(0);
    const unsigned int n(labels.size());
    for (unsigned int i = 0; i != n; ++i) {
      if (labels[i] == defaultLabel)
        set = i;
    }
    map<string, vector<unsigned int>> table;
    CMS_SA_ALLOW if (iPS.existsAs<vector<ParameterSet>>("prescaleTable", true)) {
      const vector<ParameterSet>& vpTable(iPS.getParameterSetVector("prescaleTable"));
      const unsigned int m(vpTable.size());
      for (unsigned int i = 0; i != m; ++i) {
        table[vpTable[i].getParameter<std::string>("pathName")] =
            vpTable[i].getParameter<std::vector<unsigned int>>("prescales");
      }
    }
    if (n > 0) {
      hltPrescaleTable_ = HLTPrescaleTable(set, labels, table);
    } else {
      hltPrescaleTable_ = HLTPrescaleTable();
    }
  }

  // fill maps to return prescales values with allowed types (double, FractionalPrescale)
  for (auto const& [key, psVals] : hltPrescaleTable_.table()) {
    hltPrescaleTableValuesDouble_.insert(
        {key, edm::vector_transform(psVals, [](auto const ps) -> double { return ps; })});
    hltPrescaleTableValuesFractional_.insert(
        {key, edm::vector_transform(psVals, [](auto const ps) -> FractionalPrescale { return ps; })});
  }

  // Determine L1T Type (0=unknown, 1=legacy/stage-1 or 2=stage-2)
  l1tType_ = 0;
  unsigned int stage1(0), stage2(0);
  CMS_SA_ALLOW if (processPSet_->existsAs<std::vector<std::string>>("@all_modules")) {
    const std::vector<std::string>& allModules(processPSet_->getParameter<std::vector<std::string>>("@all_modules"));
    for (auto const& allModule : allModules) {
      if ((moduleType(allModule) == "HLTLevel1GTSeed") or (moduleType(allModule) == "L1GlobalTrigger")) {
        stage1 += 1;
      } else if ((moduleType(allModule) == "HLTL1TSeed") or (moduleType(allModule) == "L1TGlobalProducer")) {
        stage2 += 1;
      }
    }
  }
  if ((stage1 + stage2) == 0) {
    l1tType_ = 0;
    //     edm::LogError("HLTConfigData") << " Can't identify l1tType: Process '" << processName_ << "' does not contain any identifying instances!";
  } else if ((stage1 * stage2) != 0) {
    l1tType_ = 0;
    //     edm::LogError("HLTConfigData") << " Can't identify l1tType: Process '" << processName_ << "' contains both legacy/stage-1/stage-2 instances!";
  } else if (stage1 > 0) {
    l1tType_ = 1;
    //     edm::LogError("HLTConfigData") << " Identified Process '" << processName_ << "' as legacy/stage-1 L1T!";
  } else {
    l1tType_ = 2;
    //     edm::LogError("HLTConfigData") << " Identified Process '" << processName_ << "' as stage-2 L1T!";
  }

  LogVerbatim("HLTConfigData") << "HLTConfigData: ProcessPSet with name/GT/table/l1tType: '" << processName_ << "' '"
                               << globalTag_ << "' '" << tableName_ << "' " << l1tType_;

  return;
}

void HLTConfigData::dump(const std::string& what) const {
  using namespace std;
  using namespace edm;

  if (what == "ProcessPSet") {
    cout << "HLTConfigData::dump: ProcessPSet = " << endl << *processPSet_ << endl;
  } else if (what == "ProcessName") {
    cout << "HLTConfigData::dump: ProcessName = " << processName_ << endl;
  } else if (what == "GlobalTag") {
    cout << "HLTConfigData::dump: GlobalTag = " << globalTag_ << endl;
  } else if (what == "TableName") {
    cout << "HLTConfigData::dump: TableName = " << tableName_ << endl;
  } else if (what == "Triggers") {
    const unsigned int n(size());
    cout << "HLTConfigData::dump: Triggers: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      cout << "  " << i << " " << triggerNames_[i] << endl;
    }
  } else if (what == "TriggerSeeds") {
    const unsigned int n(size());
    cout << "HLTConfigData::dump: TriggerSeeds: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      const unsigned int m1(hltL1GTSeeds_[i].size());
      const unsigned int m2(hltL1TSeeds_[i].size());
      cout << "  " << i << " " << triggerNames_[i] << " " << m1 << "/" << m2 << endl;
      if (m1 > 0) {
        for (unsigned int j1 = 0; j1 != m1; ++j1) {
          cout << "    HLTLevel1GTSeed: " << j1 << " " << hltL1GTSeeds_[i][j1].first << "/"
               << hltL1GTSeeds_[i][j1].second;
        }
        cout << endl;
      }
      if (m2 > 0) {
        for (unsigned int j2 = 0; j2 != m2; ++j2) {
          cout << "    HLTL1TSeed: " << j2 << " " << hltL1TSeeds_[i][j2];
        }
        cout << endl;
      }
    }
  } else if (what == "Modules") {
    const unsigned int n(size());
    cout << "HLTConfigData::dump Triggers and Modules: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      const unsigned int m(size(i));
      cout << i << " " << triggerNames_[i] << " " << m << endl;
      cout << " - Modules: ";
      unsigned int nHLTPrescalers(0);
      unsigned int nHLTLevel1GTSeed(0);
      unsigned int nHLTL1TSeed(0);
      for (unsigned int j = 0; j != m; ++j) {
        const string& label(moduleLabels_[i][j]);
        const string type(moduleType(label));
        const string edmtype(moduleEDMType(label));
        const bool tags(saveTags(label));
        cout << " " << j << ":" << label << "/" << type << "/" << edmtype << "/" << tags;
        if (type == "HLTPrescaler")
          nHLTPrescalers++;
        if (type == "HLTLevel1GTSeed")
          nHLTLevel1GTSeed++;
        if (type == "HLTL1TSeed")
          nHLTL1TSeed++;
      }
      cout << endl;
      cout << " - Number of HLTPrescaler/HLTLevel1GTSeed/HLTL1TSeed modules: " << nHLTPrescalers << "/"
           << nHLTLevel1GTSeed << "/" << nHLTL1TSeed << endl;
    }
  } else if (what == "StreamNames") {
    const unsigned int n(streamNames_.size());
    cout << "HLTConfigData::dump: StreamNames: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      cout << "  " << i << " " << streamNames_[i] << endl;
    }
  } else if (what == "Streams") {
    const unsigned int n(streamNames_.size());
    cout << "HLTConfigData::dump: Streams: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      const unsigned int m(streamContents_[i].size());
      cout << "  " << i << " " << streamNames_[i] << " " << m << endl;
      for (unsigned int j = 0; j != m; ++j) {
        cout << "    " << j << " " << streamContents_[i][j] << endl;
      }
    }
  } else if (what == "DatasetNames") {
    const unsigned int n(datasetNames_.size());
    cout << "HLTConfigData::dump: DatasetNames: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      cout << "  " << i << " " << datasetNames_[i] << endl;
    }
  } else if (what == "Datasets") {
    const unsigned int n(datasetNames_.size());
    cout << "HLTConfigData::dump: Datasets: " << n << endl;
    for (unsigned int i = 0; i != n; ++i) {
      const unsigned int m(datasetContents_[i].size());
      cout << "  " << i << " " << datasetNames_[i] << " " << m << endl;
      for (unsigned int j = 0; j != m; ++j) {
        cout << "    " << j << " " << datasetContents_[i][j] << endl;
      }
    }
  } else if (what == "PrescaleTable") {
    const unsigned int n(hltPrescaleTable_.size());
    cout << "HLTConfigData::dump: PrescaleTable: # of sets : " << n << endl;
    const vector<string>& labels(hltPrescaleTable_.labels());
    for (unsigned int i = 0; i != n; ++i) {
      cout << " " << i << "/'" << labels.at(i) << "'";
    }
    if (n > 0)
      cout << endl;
    auto const& table = hltPrescaleTable_.table();
    cout << "HLTConfigData::dump: PrescaleTable: # of paths: " << table.size() << endl;
    for (auto const& [key, val] : table) {
      for (unsigned int i = 0; i != n; ++i) {
        cout << " " << val.at(i);
      }
      cout << " " << key << endl;
    }
  } else {
    cout << "HLTConfigData::dump: Unkown dump request: " << what << endl;
  }
  return;
}

const std::string& HLTConfigData::processName() const { return processName_; }

const std::string& HLTConfigData::globalTag() const { return globalTag_; }

unsigned int HLTConfigData::size() const { return triggerNames_.size(); }
unsigned int HLTConfigData::size(unsigned int trigger) const { return moduleLabels_.at(trigger).size(); }
unsigned int HLTConfigData::size(const std::string& trigger) const { return size(triggerIndex(trigger)); }

const std::string& HLTConfigData::tableName() const { return tableName_; }
const std::vector<std::string>& HLTConfigData::triggerNames() const { return triggerNames_; }
const std::string& HLTConfigData::triggerName(unsigned int trigger) const { return triggerNames_.at(trigger); }
unsigned int HLTConfigData::triggerIndex(const std::string& trigger) const {
  const std::map<std::string, unsigned int>::const_iterator index(triggerIndex_.find(trigger));
  if (index == triggerIndex_.end()) {
    return size();
  } else {
    return index->second;
  }
}

const std::vector<std::string>& HLTConfigData::moduleLabels(unsigned int trigger) const {
  return moduleLabels_.at(trigger);
}
const std::vector<std::string>& HLTConfigData::moduleLabels(const std::string& trigger) const {
  return moduleLabels_.at(triggerIndex(trigger));
}

const std::vector<std::string>& HLTConfigData::saveTagsModules(unsigned int trigger) const {
  return saveTagsModules_.at(trigger);
}
const std::vector<std::string>& HLTConfigData::saveTagsModules(const std::string& trigger) const {
  return saveTagsModules_.at(triggerIndex(trigger));
}

const std::string& HLTConfigData::moduleLabel(unsigned int trigger, unsigned int module) const {
  return moduleLabels_.at(trigger).at(module);
}
const std::string& HLTConfigData::moduleLabel(const std::string& trigger, unsigned int module) const {
  return moduleLabels_.at(triggerIndex(trigger)).at(module);
}

unsigned int HLTConfigData::moduleIndex(unsigned int trigger, const std::string& module) const {
  const std::map<std::string, unsigned int>::const_iterator index(moduleIndex_.at(trigger).find(module));
  if (index == moduleIndex_.at(trigger).end()) {
    return size(trigger);
  } else {
    return index->second;
  }
}
unsigned int HLTConfigData::moduleIndex(const std::string& trigger, const std::string& module) const {
  return moduleIndex(triggerIndex(trigger), module);
}

const std::string& HLTConfigData::moduleType(const std::string& module) const { return moduleInfoFor(module).class_; }

const std::string& HLTConfigData::moduleEDMType(const std::string& module) const {
  return moduleInfoFor(module).edmType_;
}

const edm::ParameterSet& HLTConfigData::processPSet() const { return *processPSet_; }

const edm::ParameterSet& HLTConfigData::modulePSet(const std::string& module) const {
  //HLTConfigProvider sees ignored modules as "-modname"
  //but in the PSet, the module is named "modname"
  //so if it starts with "-", you need to remove the "-" from the
  //module name to be able to retreive it from the PSet
  return *(moduleInfoFor(module).pset_);
}
const HLTConfigData::ModuleInfo& HLTConfigData::moduleInfoFor(const std::string& module) const {
  auto canon = canonicalModuleName(module);
  auto found = modulesInfo_.find(canon);
  if (found != modulesInfo_.end()) {
    return found->second;
  }
  CMS_SA_ALLOW if (processPSet_->exists(canon)) {
    auto const& pset = processPSet_->getParameterSet(canon);
    std::string class_;
    std::string type;
    CMS_SA_ALLOW if (pset.existsAs<std::string>("@module_type", true)) {
      class_ = pset.getParameter<std::string>("@module_type");
    }
    CMS_SA_ALLOW if (pset.existsAs<std::string>("@module_edm_type", true)) {
      type = pset.getParameter<std::string>("@module_edm_type");
    }
    found = modulesInfo_.emplace(module, ModuleInfo{&pset, class_, type}).first;
  }
  else {
    found = modulesInfo_.emplace(module, ModuleInfo{s_dummyPSet(), "", ""}).first;
  }
  return found->second;
}

bool HLTConfigData::saveTags(const std::string& module) const {
  const edm::ParameterSet& pset(modulePSet(module));
  CMS_SA_ALLOW if (pset.existsAs<bool>("saveTags", true)) { return pset.getParameter<bool>("saveTags"); }
  else {
    return false;
  }
}

unsigned int HLTConfigData::l1tType() const { return l1tType_; }

const std::vector<std::vector<std::pair<bool, std::string>>>& HLTConfigData::hltL1GTSeeds() const {
  return hltL1GTSeeds_;
}

const std::vector<std::pair<bool, std::string>>& HLTConfigData::hltL1GTSeeds(const std::string& trigger) const {
  return hltL1GTSeeds(triggerIndex(trigger));
}

const std::vector<std::pair<bool, std::string>>& HLTConfigData::hltL1GTSeeds(unsigned int trigger) const {
  return hltL1GTSeeds_.at(trigger);
}

const std::vector<std::vector<std::string>>& HLTConfigData::hltL1TSeeds() const { return hltL1TSeeds_; }

const std::vector<std::string>& HLTConfigData::hltL1TSeeds(const std::string& trigger) const {
  return hltL1TSeeds(triggerIndex(trigger));
}

const std::vector<std::string>& HLTConfigData::hltL1TSeeds(unsigned int trigger) const {
  return hltL1TSeeds_.at(trigger);
}

/// Streams
const std::vector<std::string>& HLTConfigData::streamNames() const { return streamNames_; }

const std::string& HLTConfigData::streamName(unsigned int stream) const { return streamNames_.at(stream); }

unsigned int HLTConfigData::streamIndex(const std::string& stream) const {
  const std::map<std::string, unsigned int>::const_iterator index(streamIndex_.find(stream));
  if (index == streamIndex_.end()) {
    return streamNames_.size();
  } else {
    return index->second;
  }
}

const std::vector<std::vector<std::string>>& HLTConfigData::streamContents() const { return streamContents_; }

const std::vector<std::string>& HLTConfigData::streamContent(unsigned int stream) const {
  return streamContents_.at(stream);
}

const std::vector<std::string>& HLTConfigData::streamContent(const std::string& stream) const {
  return streamContent(streamIndex(stream));
}

/// Datasets
const std::vector<std::string>& HLTConfigData::datasetNames() const { return datasetNames_; }

const std::string& HLTConfigData::datasetName(unsigned int dataset) const { return datasetNames_.at(dataset); }

unsigned int HLTConfigData::datasetIndex(const std::string& dataset) const {
  const std::map<std::string, unsigned int>::const_iterator index(datasetIndex_.find(dataset));
  if (index == datasetIndex_.end()) {
    return datasetNames_.size();
  } else {
    return index->second;
  }
}

const std::vector<std::vector<std::string>>& HLTConfigData::datasetContents() const { return datasetContents_; }

const std::vector<std::string>& HLTConfigData::datasetContent(unsigned int dataset) const {
  return datasetContents_.at(dataset);
}

const std::vector<std::string>& HLTConfigData::datasetContent(const std::string& dataset) const {
  return datasetContent(datasetIndex(dataset));
}

unsigned int HLTConfigData::prescaleSize() const { return hltPrescaleTable_.size(); }

template <>
std::map<std::string, std::vector<double>> const& HLTConfigData::prescaleTable() const {
  return hltPrescaleTableValuesDouble_;
}

template <>
std::map<std::string, std::vector<FractionalPrescale>> const& HLTConfigData::prescaleTable() const {
  return hltPrescaleTableValuesFractional_;
}

const std::vector<std::string>& HLTConfigData::prescaleLabels() const { return hltPrescaleTable_.labels(); }

edm::ParameterSetID HLTConfigData::id() const { return processPSet_->id(); }
