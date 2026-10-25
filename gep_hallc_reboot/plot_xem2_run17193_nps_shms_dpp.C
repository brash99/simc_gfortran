#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TRotation.h>
#include <TStyle.h>
#include <TTree.h>
#include <TVector3.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kMp = 0.9382720813;
constexpr double kEbeam = 10.551;
constexpr double kP0 = 6.476;

double ProtonMomentumFromTheta(double theta) {
  const double c = std::cos(theta);
  const double den = (kEbeam+kMp)*(kEbeam+kMp)-kEbeam*kEbeam*c*c;
  return 2.*kMp*kEbeam*(kEbeam+kMp)*c/den;
}

TVector3 ToLab(double p, double xptar, double yptar, double theta_deg) {
  const double th=theta_deg*kPi/180.;
  const double st=std::sin(th), ct=std::cos(th);
  TVector3 nx(0.,-1.,0.), ny(ct,0.,-st), nz(st,0.,ct);
  TRotation rot; rot.SetToIdentity(); rot.RotateAxes(nx,ny,nz);
  TVector3 v(xptar,yptar,1.);
  v *= p/std::sqrt(1.+xptar*xptar+yptar*yptar);
  return rot*v;
}

void FillDpp(TTree* t, TH1D& h, double scale, bool require_shms,
             bool one_crystal_margin=false) {
  Float_t hsdelta=0,hsxptar=0,hsyptar=0,ssdelta=0,ssxfp=0,ssyfp=0,weight=0;
  t->SetBranchAddress("hsdelta",&hsdelta);
  t->SetBranchAddress("hsxptar",&hsxptar);
  t->SetBranchAddress("hsyptar",&hsyptar);
  t->SetBranchAddress("ssdelta",&ssdelta);
  t->SetBranchAddress("ssxfp",&ssxfp);
  t->SetBranchAddress("ssyfp",&ssyfp);
  t->SetBranchAddress("Weight",&weight);
  for (Long64_t i=0;i<t->GetEntries();++i) {
    t->GetEntry(i);
    if (require_shms && (ssdelta<=-10. || ssdelta>=22.)) continue;
    if (one_crystal_margin &&
        (std::abs(ssyfp)>=28.70 || std::abs(ssxfp)>=34.85)) continue;
    const double p=kP0*(1.+hsdelta/100.);
    const TVector3 v=ToLab(p,hsxptar,hsyptar,-19.52);
    const double dpp=100.*(p-ProtonMomentumFromTheta(v.Theta()))/kP0;
    h.Fill(dpp,weight*scale);
  }
}
}

void plot_xem2_run17193_nps_shms_dpp() {
  TFile fshms("/work/hallc/gep/brash/simc_xem2_run17193/worksim/"
              "xem2_run17193_elastic_30cm.root","READ");
  TFile fnps("/work/hallc/gep/brash/simc_xem2_run17193/worksim/"
             "xem2_run17193_nps_30cm_3m_10M.root","READ");
  auto* shms=dynamic_cast<TTree*>(fshms.Get("h10"));
  auto* nps=dynamic_cast<TTree*>(fnps.Get("h10"));
  if (!shms || !nps) { std::cerr << "Missing SIMC tree\n"; return; }

  TH1D hshms("hshms","",64,-4.,4.);
  TH1D hnps("hnps","",64,-4.,4.);
  TH1D hnpsfid("hnpsfid","",64,-4.,4.);
  for (auto* h:{&hshms,&hnps,&hnpsfid}) h->Sumw2();
  FillDpp(shms,hshms,1.77187e8/1.e6,true);
  FillDpp(nps,hnps,8.15694e9/1.e7,false);
  FillDpp(nps,hnpsfid,8.15694e9/1.e7,false,true);

  gStyle->SetOptStat(0);
  TCanvas c("c","GEp-III delta p_p with NPS",950,700);
  c.SetLeftMargin(.13); c.SetBottomMargin(.13); c.SetTopMargin(.09);
  hnps.SetTitle("GEp-III-style #delta p_{p}: NPS versus SHMS;"
                "#delta p_{p} = 100[p_{p}-p_{p}(#theta_{p})]/p_{0}  (%);"
                "Events / mC / 0.125%");
  hnps.SetLineColor(kBlue+1); hnps.SetLineWidth(3);
  hnpsfid.SetLineColor(kCyan+2); hnpsfid.SetLineStyle(2); hnpsfid.SetLineWidth(3);
  hshms.SetLineColor(kRed+1); hshms.SetLineWidth(3);
  hnps.SetMaximum(1.18*std::max({hnps.GetMaximum(),hnpsfid.GetMaximum(),hshms.GetMaximum()}));
  hnps.Draw("HIST"); hnpsfid.Draw("HIST SAME"); hshms.Draw("HIST SAME");
  TLegend leg(.56,.64,.89,.82); leg.SetBorderSize(0); leg.SetFillStyle(0);
  leg.AddEntry(&hnps,"NPS physical boundary","l");
  leg.AddEntry(&hnpsfid,"NPS one-crystal margin","l");
  leg.AddEntry(&hshms,"SHMS","l"); leg.Draw();
  TLatex tx; tx.SetNDC(); tx.SetTextSize(.034);
  tx.DrawLatex(.16,.87,"Run-17193 kinematics; 30 cm LH_{2}; absolute normalization");
  c.SaveAs("xem2_run17193_nps_shms_dpp.pdf");
  c.SaveAs("xem2_run17193_nps_shms_dpp.png");

  std::ofstream out("xem2_run17193_nps_shms_dpp_summary.csv");
  out << "quantity,value\n" << std::setprecision(12)
      << "shms_visible_yield_events_per_mC," << hshms.Integral() << "\n"
      << "nps_visible_yield_events_per_mC," << hnps.Integral() << "\n"
      << "nps_fid_visible_yield_events_per_mC," << hnpsfid.Integral() << "\n"
      << "nps_over_shms_visible," << hnps.Integral()/hshms.Integral() << "\n";
}
