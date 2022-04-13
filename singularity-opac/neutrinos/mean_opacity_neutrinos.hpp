// ======================================================================
// © 2022. Triad National Security, LLC. All rights reserved.  This
// program was produced under U.S. Government contract
// 89233218CNA000001 for Los Alamos National Laboratory (LANL), which
// is operated by Triad National Security, LLC for the U.S.
// Department of Energy/National Nuclear Security Administration. All
// rights in the program are reserved by Triad National Security, LLC,
// and the U.S. Department of Energy/National Nuclear Security
// Administration. The Government is granted for itself and others
// acting on its behalf a nonexclusive, paid-up, irrevocable worldwide
// license in this material to reproduce, prepare derivative works,
// distribute copies to the public, perform publicly and display
// publicly, and to permit others to do so.
// ======================================================================

#ifndef SINGULARITY_OPAC_NEUTRINOS_MEAN_OPACITY_NEUTRINOS_
#define SINGULARITY_OPAC_NEUTRINOS_MEAN_OPACITY_NEUTRINOS_

#include <cmath>

#include <ports-of-call/portability.hpp>
#include <singularity-opac/base/radiation_types.hpp>
#include <singularity-opac/base/sp5.hpp>
#include <singularity-opac/constants/constants.hpp>
#include <spiner/databox.hpp>

namespace singularity {
namespace neutrinos {

using pc = PhysicalConstants<CGS>;

#define EPS (10.0 * std::numeric_limits<Real>::min())

// TODO(BRR) Note: It is assumed that lambda is constant for all densities,
// temperatures, and Ye

class MeanOpacity {
 public:
  MeanOpacity() = default;
  template <typename Opacity>
  MeanOpacity(const Opacity &opac, Real lRhoMin, Real lRhoMax, int NRho,
              Real lTMin, Real lTMax, int NT, Real YeMin, Real YeMax, int NYe,
              Real *lambda = nullptr) {
    nlambda_ = opac.nlambda();

    lkappaPlanck_.resize(NRho, NT, NYe, NEUTRINO_NTYPES);
    // index 0 is the species and is not interpolatable
    lkappaPlanck_.setRange(1, YeMin, YeMax, NYe);
    lkappaPlanck_.setRange(2, lTMin, lTMax, NT);
    lkappaPlanck_.setRange(3, lRhoMin, lRhoMax, NRho);
    lkappaRosseland_.copyMetadata(lkappaPlanck_);

    // Fill tables
    for (int iRho = 0; iRho < NRho; ++iRho) {
      Real lRho = lkappaPlanck_.range(3).x(iRho);
      Real rho = fromLog_(lRho);
      for (int iT = 0; iT < NT; ++iT) {
        Real lT = lkappaPlanck_.range(2).x(iT);
        Real T = fromLog_(lT);
        for (int iYe = 0; iYe < NYe; ++iYe) {
          Real Ye = lkappaPlanck_.range(1).x(iYe);
          for (int idx = 0; idx < NEUTRINO_NTYPES; ++idx) {
            RadiationType type = Idx2RadType(idx);
            Real kappaPlanckNum = 0.;
            Real kappaPlanckDenom = 0.;
            Real kappaRosselandNum = 0.;
            Real kappaRosselandDenom = 0.;
            // Integrate over frequency
            const int nnu = 100;
            const Real lnuMin = toLog_(1.e10);
            const Real lnuMax = toLog_(1.e30);
            const Real dlnu = (lnuMax - lnuMin) / (nnu - 1);
            for (int inu = 0; inu < nnu; ++inu) {
              const Real lnu = lnuMin + inu * dlnu;
              const Real nu = fromLog_(lnu);
              kappaPlanckNum +=
                  opac.AbsorptionCoefficient(rho, T, Ye, type, nu, lambda) /
                  rho * opac.ThermalDistributionOfTNu(T, type, nu) * nu * dlnu;
              kappaPlanckDenom +=
                  opac.ThermalDistributionOfTNu(T, type, nu) * nu * dlnu;

              kappaRosselandNum +=
                  rho /
                  opac.AbsorptionCoefficient(rho, T, Ye, type, nu, lambda) *
                  opac.DThermalDistributionOfTNuDT(T, type, nu) * nu * dlnu;
              kappaRosselandDenom +=
                  opac.DThermalDistributionOfTNuDT(T, type, nu) * nu * dlnu;
            }

            // Trapezoidal rule
            const Real nu0 = fromLog_(lnuMin);
            const Real nu1 = fromLog_(lnuMax);
            kappaPlanckNum -=
                0.5 * 1. / rho *
                (opac.AbsorptionCoefficient(rho, T, Ye, type, nu0, lambda) *
                     opac.ThermalDistributionOfTNu(T, type, nu0) * nu0 +
                 opac.AbsorptionCoefficient(rho, T, Ye, type, nu1, lambda) *
                     opac.ThermalDistributionOfTNu(T, type, nu1) * nu1) *
                dlnu;
            kappaPlanckDenom -=
                0.5 *
                (opac.ThermalDistributionOfTNu(T, type, nu0) * nu0 +
                 opac.ThermalDistributionOfTNu(T, type, nu1) * nu1) *
                dlnu;
            kappaRosselandNum -=
                0.5 * rho *
                (1. /
                     opac.AbsorptionCoefficient(rho, T, Ye, type, nu0, lambda) *
                     opac.DThermalDistributionOfTNuDT(T, type, nu0) * nu0 +
                 1. /
                     opac.AbsorptionCoefficient(rho, T, Ye, type, nu1, lambda) *
                     opac.DThermalDistributionOfTNuDT(T, type, nu1) * nu1) *
                dlnu;
            kappaRosselandDenom -=
                0.5 *
                (opac.DThermalDistributionOfTNuDT(T, type, nu0) * nu0 +
                 opac.DThermalDistributionOfTNuDT(T, type, nu1) * nu1) *
                dlnu;

            Real lkappaPlanck = toLog_(kappaPlanckNum / kappaPlanckDenom);
            Real lkappaRosseland =
                toLog_(1. / (kappaRosselandNum / kappaRosselandDenom));
            lkappaPlanck_(iRho, iT, iYe, idx) = lkappaPlanck;
            lkappaRosseland_(iRho, iT, iYe, idx) = lkappaRosseland;
          }
        }
      }
    }
  }

#ifdef SPINER_USE_HDF
  MeanOpacity(const std::string &filename) : filename_(filename.c_str()) {
    herr_t status = H5_SUCCESS;
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    status += lkappaPlanck_.loadHDF(file, SP5::MeanOpac::PlanckMeanOpacity);
    status +=
        lkappaRosseland_.loadHDF(file, SP5::MeanOpac::RosselandMeanOpacity);
    status += H5Fclose(file);

    if (status != H5_SUCCESS) {
      OPAC_ERROR("neutrinos::MeanOpacity: HDF5 error\n");
    }
  }

  void Save(const std::string &filename) const {
    herr_t status = H5_SUCCESS;
    hid_t file =
        H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    status += lkappaPlanck_.saveHDF(file, SP5::MeanOpac::PlanckMeanOpacity);
    status +=
        lkappaRosseland_.saveHDF(file, SP5::MeanOpac::RosselandMeanOpacity);
    status += H5Fclose(file);

    if (status != H5_SUCCESS) {
      OPAC_ERROR("neutrinos::MeanOpacity: HDF5 error\n");
    }
  }
#endif

  PORTABLE_INLINE_FUNCTION int nlambda() const { return nlambda_; }

  PORTABLE_INLINE_FUNCTION void PrintParams() const {
    printf("Mean opacity\n");
  }

  MeanOpacity GetOnDevice() {
    MeanOpacity other;
    other.nlambda_ = nlambda_;
    other.lkappaPlanck_ = Spiner::getOnDeviceDataBox(lkappaPlanck_);
    other.lkappaRosseland_ = Spiner::getOnDeviceDataBox(lkappaRosseland_);
    return other;
  }

  void Finalize() {
    lkappaPlanck_.finalize();
    lkappaRosseland_.finalize();
  }

  PORTABLE_INLINE_FUNCTION
  Real PlanckMeanAbsorptionCoefficient(const Real rho, const Real temp,
                                       const Real Ye,
                                       const RadiationType type) const {
    Real lRho = toLog_(rho);
    Real lT = toLog_(temp);
    int idx = RadType2Idx(type);
    return rho * fromLog_(lkappaPlanck_.interpToReal(lRho, lT, Ye, idx));
  }

  PORTABLE_INLINE_FUNCTION
  Real RosselandMeanAbsorptionCoefficient(const Real rho, const Real temp,
                                          const Real Ye,
                                          const RadiationType type) const {
    Real lRho = toLog_(rho);
    Real lT = toLog_(temp);
    int idx = RadType2Idx(type);
    return rho * fromLog_(lkappaRosseland_.interpToReal(lRho, lT, Ye, idx));
  }

 private:
  PORTABLE_INLINE_FUNCTION Real toLog_(const Real x) const {
    return std::log10(std::abs(x) + EPS);
  }
  PORTABLE_INLINE_FUNCTION Real fromLog_(const Real lx) const {
    return std::pow(10., lx);
  }
  Spiner::DataBox lkappaPlanck_;
  Spiner::DataBox lkappaRosseland_;
  int nlambda_;
  const char *filename_;
};

} // namespace neutrinos
} // namespace singularity

#endif // SINGULARITY_OPAC_NEUTRINOS_MEAN_OPACITY_NEUTRINOS__
