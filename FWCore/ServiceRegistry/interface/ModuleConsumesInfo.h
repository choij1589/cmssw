#ifndef FWCore_ServiceRegistry_ModuleConsumesInfo_h
#define FWCore_ServiceRegistry_ModuleConsumesInfo_h

/**\class edm::ModuleConsumesInfo

   Description: Contains information about a product
   a module will get (consume).

   Usage: These are typically returned by the PathsAndConsumesOfModules
   object available in selected service callbacks.
*/
//
// Original Author: W. David Dagenhart
//         Created: 12/4/2014

#include "FWCore/Utilities/interface/BranchType.h"
#include "FWCore/Utilities/interface/ProductKindOfType.h"
#include "FWCore/Utilities/interface/TypeID.h"

#include <string_view>

namespace edm {
  class ModuleConsumesInfo {
  public:
    ModuleConsumesInfo(TypeID const& iType,
                       char const* iLabel,
                       char const* iInstance,
                       char const* iProcess,
                       BranchType iBranchType,
                       KindOfType iKindOfType,
                       bool iAlwaysGets,
                       bool iSkipCurrentProcess_);

    TypeID const& type() const { return type_; }
    std::string_view label() const { return label_; }
    std::string_view instance() const { return instance_; }
    std::string_view process() const { return process_; }
    BranchType branchType() const { return branchType_; }
    KindOfType kindOfType() const { return kindOfType_; }
    bool alwaysGets() const { return alwaysGets_; }
    bool skipCurrentProcess() const { return skipCurrentProcess_; }

    // This provides information from EDConsumerBase
    //
    // process is empty - A get will search over processes in reverse
    //    time order (unknown which process the product will be gotten
    //    from and it is possible for this to vary from event to event)

  private:
    TypeID type_;
    std::string_view label_;
    std::string_view instance_;
    std::string_view process_;
    BranchType branchType_;
    KindOfType kindOfType_;
    bool alwaysGets_;
    bool skipCurrentProcess_;
  };
}  // namespace edm
#endif
