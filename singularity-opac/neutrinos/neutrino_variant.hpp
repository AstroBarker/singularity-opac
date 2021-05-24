// ======================================================================
// © 2021. Triad National Security, LLC. All rights reserved.  This
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

#ifndef SINGULARITY_OPAC_NEUTRINOS_NEUTRINO_VARIANT_
#define SINGULARITY_OPAC_NEUTRINOS_NEUTRINO_VARIANT_

#include <utility>

#include <ports-of-call/portability.hpp>
#include <singularity-opac/base/opac_error.hpp>
#include <singularity-opac/base/radiation_types.hpp>
#include <variant/include/mpark/variant.hpp>

namespace singularity {
namespace neutrinos {
namespace impl {

template <typename... Ts>
using opac_variant = mpark::variant<Ts...>;

template <typename... Opacs>
class Variant {
 private:
  opac_variant<Opacs...> opac_;

 public:
  template <
      typename Choice,
      typename std::enable_if<
          !std::is_same<Variant, typename std::decay<Choice>::type>::value,
          bool>::type = true>
  PORTABLE_FUNCTION Variant(Choice &&choice)
      : opac_(std::forward<Choice>(choice)) {}

  PORTABLE_FUNCTION
  Variant() noexcept = default;

  template <
      typename Choice,
      typename std::enable_if<
          !std::is_same<Variant, typename std::decay<Choice>::type>::value,
          bool>::type = true>
  PORTABLE_FUNCTION Variant &operator=(Choice &&opac) {
    opac_ = std::forward<Choice>(opac);
    return *this;
  }

  template <
      typename Choice,
      typename std::enable_if<
          !std::is_same<Variant, typename std::decay<Choice>::type>::value,
          bool>::type = true>
  Choice get() {
    return mpark::get<Choice>(opac_);
  }

  Variant GetOnDevice() {
    return mpark::visit(
        [](auto &opac) { return opac_variant<Opacs...>(opac.GetOnDevice()); },
        opac_);
  }

  // opacity
  // Signature should be at least
  // rho, temp, nu, lambda
  PORTABLE_INLINE_FUNCTION Real AbsorptionCoefficientPerNu(
      const RadiationType type, const Real rho, const Real temp, const Real Ye,
      const Real nu, Real *lambda = nullptr) const {
    return mpark::visit(
        [=](const auto &opac) {
          return opac.AbsorptionCoefficientPerNu(type, rho, temp, Ye, nu,
                                                 lambda);
        },
        opac_);
  }

  template <typename FrequencyIndexer, typename DataIndexer>
  PORTABLE_INLINE_FUNCTION void AbsorptionCoefficientPerNu(
      const RadiationType type, const Real rho, const Real temp, const Real Ye,
      const FrequencyIndexer &nu_bins, DataIndexer &coeffs, const int nbins,
      Real *lambda = nullptr) const {
    mpark::visit(
        [=](const auto &opac) {
          opac.AbsorptionCoefficientPerNu(type, rho, temp, Ye, nu_bins, coeffs,
                                          nbins);
        },
        opac_);
  }

  // emissivity with units of energy/time/frequency/volume/angle
  // signature should be at least rho, temp, nu, lambda
  PORTABLE_INLINE_FUNCTION Real EmissivityPerNuOmega(
      const RadiationType type, const Real rho, const Real temp, const Real Ye,
      const Real nu, Real *lambda = nullptr) const {
    return mpark::visit(
        [=](const auto &opac) {
          return opac.EmissivityPerNuOmega(type, rho, temp, Ye, nu, lambda);
        },
        opac_);
  }

  template <typename FrequencyIndexer, typename DataIndexer>
  PORTABLE_INLINE_FUNCTION void
  EmissivityPerNuOmega(const RadiationType type, const Real rho,
                       const Real temp, const Real Ye,
                       const FrequencyIndexer &nu_bins, DataIndexer &coeffs,
                       const int nbins, Real *lambda = nullptr) const {
    mpark::visit(
        [=](const auto &opac) {
          return opac.EmissivityPerNuOmega(type, rho, temp, Ye, nu_bins, coeffs,
                                           nbins, lambda);
        },
        opac_);
  }

  // emissivity integrated over angle
  PORTABLE_INLINE_FUNCTION Real EmissivityPerNu(const RadiationType type,
                                                const Real rho, const Real temp,
                                                const Real Ye, const Real nu,
                                                Real *lambda = nullptr) const {
    return mpark::visit(
        [=](const auto &opac) {
          return opac.EmissivityPerNu(type, rho, temp, Ye, nu, lambda);
        },
        opac_);
  }

  template <typename FrequencyIndexer, typename DataIndexer>
  PORTABLE_INLINE_FUNCTION void
  EmissivityPerNu(const RadiationType type, const Real rho, const Real temp,
                  const Real Ye, const FrequencyIndexer &nu_bins,
                  DataIndexer &coeffs, const int nbins,
                  Real *lambda = nullptr) const {
    mpark::visit(
        [=](const auto &opac) {
          return opac.EmissivityPerNu(type, rho, temp, Ye, nu_bins, coeffs,
                                      nbins, lambda);
        },
        opac_);
  }

  // emissivity integrated over angle and frequency
  PORTABLE_INLINE_FUNCTION Real Emissivity(const RadiationType type,
                                           const Real rho, const Real temp,
                                           const Real Ye,
                                           Real *lambda = nullptr) const {
    return mpark::visit(
        [=](const auto &opac) {
          return opac.Emissivity(type, rho, temp, Ye, lambda);
        },
        opac_);
  }

  // Emissivity of packet
  PORTABLE_INLINE_FUNCTION Real NumberEmissivity(RadiationType type,
                                                 const Real rho,
                                                 const Real temp, Real Ye,
                                                 Real *lambda = nullptr) const {
    return mpark::visit(
        [=](const auto &opac) {
          return opac.NumberEmissivity(type, rho, temp, Ye, lambda);
        },
        opac_);
  }

  PORTABLE_INLINE_FUNCTION
  int nlambda() const noexcept {
    return mpark::visit([](const auto &opac) { return opac.nlambda(); }, opac_);
  }

  template <typename T>
  PORTABLE_INLINE_FUNCTION bool IsType() const noexcept {
    return mpark::holds_alternative<T>(opac_);
  }

  PORTABLE_INLINE_FUNCTION
  void PrintParams() const noexcept {
    return mpark::visit([](const auto &opac) { return opac.PrintParams(); },
                        opac_);
  }

  inline void Finalize() noexcept {
    return mpark::visit([](auto &opac) { return opac.Finalize(); }, opac_);
  }
};

} // namespace impl
} // namespace neutrinos
} // namespace singularity

#endif // SINGULARITY_OPAC_NEUTRINOS_NEUTRINO_VARIANT_
