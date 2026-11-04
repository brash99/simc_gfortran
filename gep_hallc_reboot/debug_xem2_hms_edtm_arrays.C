#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <iostream>

void debug_xem2_hms_edtm_arrays(
    const char* input =
        "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/HeeP/"
        "xem2_hms_hodoscope_audit_edtm_17193_100000.root") {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  TTreeReader reader(tree);
  TTreeReaderValue<UInt_t> event_type(reader, "fEvtHdr.fEvtType");
  TTreeReaderValue<Double_t> edtm(reader, "T.coin.hEDTM_tdcTime");
  TTreeReaderArray<Double_t> xpc(reader, "H.hod.1x.posTdcCounter");
  TTreeReaderArray<Double_t> xnc(reader, "H.hod.1x.negTdcCounter");
  TTreeReaderArray<Double_t> xpt(reader, "H.hod.1x.posTdcTimeRaw");
  TTreeReaderArray<Double_t> xnt(reader, "H.hod.1x.negTdcTimeRaw");
  TTreeReaderArray<Double_t> ypc(reader, "H.hod.1y.posTdcCounter");
  TTreeReaderArray<Double_t> ync(reader, "H.hod.1y.negTdcCounter");
  TTreeReaderArray<Double_t> ypt(reader, "H.hod.1y.posTdcTimeRaw");
  TTreeReaderArray<Double_t> ynt(reader, "H.hod.1y.negTdcTimeRaw");
  TTreeReaderArray<Double_t> xpg(reader, "H.hod.1x.GoodPosTdcTimeUnCorr");
  TTreeReaderArray<Double_t> xng(reader, "H.hod.1x.GoodNegTdcTimeUnCorr");
  TTreeReaderArray<Double_t> ypg(reader, "H.hod.1y.GoodPosTdcTimeUnCorr");
  TTreeReaderArray<Double_t> yng(reader, "H.hod.1y.GoodNegTdcTimeUnCorr");

  int printed = 0;
  long long nedtm = 0;
  while (reader.Next()) {
    if (*edtm == 0.0) continue;
    ++nedtm;
    if (printed++ >= 2) continue;
    std::cout << "entry=" << reader.GetCurrentEntry() << " type=" << *event_type
              << " edtm=" << *edtm << " sizes=" << xpc.GetSize() << '/'
              << xnc.GetSize() << '/' << ypc.GetSize() << '/' << ync.GetSize()
              << " good=" << xpg.GetSize() << '/' << xng.GetSize() << '/'
              << ypg.GetSize() << '/' << yng.GetSize() << '\n';
    for (size_t i = 0; i < xpc.GetSize(); ++i)
      std::cout << "  XP " << xpc[i] << " " << xpt[i] << '\n';
    for (size_t i = 0; i < xnc.GetSize(); ++i)
      std::cout << "  XN " << xnc[i] << " " << xnt[i] << '\n';
    for (size_t i = 0; i < ypc.GetSize(); ++i)
      std::cout << "  YP " << ypc[i] << " " << ypt[i] << '\n';
    for (size_t i = 0; i < ync.GetSize(); ++i)
      std::cout << "  YN " << ync[i] << " " << ynt[i] << '\n';
    for (size_t i = 0; i < xpg.GetSize(); ++i) std::cout << "  XPG " << i+1 << " " << xpg[i] << '\n';
    for (size_t i = 0; i < xng.GetSize(); ++i) std::cout << "  XNG " << i+1 << " " << xng[i] << '\n';
    for (size_t i = 0; i < ypg.GetSize(); ++i) std::cout << "  YPG " << i+1 << " " << ypg[i] << '\n';
    for (size_t i = 0; i < yng.GetSize(); ++i) std::cout << "  YNG " << i+1 << " " << yng[i] << '\n';
  }
  std::cout << "nedtm=" << nedtm << '\n';
}
