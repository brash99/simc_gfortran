#include <TFile.h>
#include <TTree.h>
#include <iostream>

void inspect_hybrid_event_mapping(
    const char *filename =
        "/work/hallc/gep/brash/gep4_nps_geant4/validation/"
        "nps_run6151_elastic_closure_2514.root") {
  TFile file(filename, "READ");
  auto *tree = dynamic_cast<TTree *>(file.Get("t_dvcs"));
  if (!tree) {
    std::cerr << "missing t_dvcs\n";
    return;
  }
  std::cout << "entries=" << tree->GetEntries() << '\n';
  for (const char *name : {"evtNb", "nps_entry", "nps_entry_id"}) {
    std::cout << name << "=" << (tree->GetBranch(name) ? "yes" : "no")
              << '\n';
  }
  tree->Scan("evtNb:nps_entry:nps_entry_id", "", "colsize=18", 5, 0);
}
