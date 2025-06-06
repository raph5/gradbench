// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

// Based on:
//   https://github.com/microsoft/ADBench/blob/38cb7931303a830c3700ca36ba9520868327ac87/src/cpp/modules/manual/gmm_d.h
//   https://github.com/microsoft/ADBench/blob/38cb7931303a830c3700ca36ba9520868327ac87/src/cpp/modules/manual/gmm_d.cpp

#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "adbench/shared/defs.h"
#include "adbench/shared/matrix.h"

#include "gradbench/evals/gmm.hpp"

void gmm_objective_d(int d, int k, int n, const double* alphas,
                     const double* means, const double* const q,
                     const double* const l, const double* x,
                     gmm::Wishart wishart, double* err, double* alpha_d,
                     double* mu_d, double* q_d, double* l_d);

void Qtransposetimesx(int d, const double* const Ldiag, const double* const icf,
                      const double* const x, double* Ltransposex) {
  int Lparamsidx = 0;
  for (int i = 0; i < d; i++)
    Ltransposex[i] = Ldiag[i] * x[i];

  for (int i = 0; i < d; i++)
    for (int j = i + 1; j < d; j++) {
      Ltransposex[i] += icf[Lparamsidx] * x[j];
      Lparamsidx++;
    }
}

void compute_q_inner_term(int d, const double* const Ldiag,
                          const double* const xcentered,
                          const double* const Lxcentered, double* logLdiag_d) {
  for (int i = 0; i < d; i++) {
    logLdiag_d[i] = 1. - Ldiag[i] * xcentered[i] * Lxcentered[i];
  }
}

void compute_L_inner_term(int d, const double* const xcentered,
                          const double* const Lxcentered, double* L_d) {
  int Lparamsidx = 0;
  for (int i = 0; i < d; i++) {
    int n_curr_elems = d - i - 1;
    for (int j = 0; j < n_curr_elems; j++) {
      L_d[Lparamsidx] = -xcentered[i] * Lxcentered[d - n_curr_elems + j];
      Lparamsidx++;
    }
  }
}

double logsumexp_d(int n, const double* const x, double* logsumexp_partial_d) {
  int    max_elem = arr_max_idx(n, x);
  double mx       = x[max_elem];
  double semx     = 0.;
  for (int i = 0; i < n; i++) {
    logsumexp_partial_d[i] = exp(x[i] - mx);
    semx += logsumexp_partial_d[i];
  }
  if (semx == 0.) {
    std::fill(logsumexp_partial_d, logsumexp_partial_d + n, 0.0);
  } else {
    logsumexp_partial_d[max_elem] -= semx;
    for (int i = 0; i < n; i++)
      logsumexp_partial_d[i] /= semx;
  }
  logsumexp_partial_d[max_elem] += 1.;
  return log(semx) + mx;
}

void gmm_objective_d(int d, int k, int n, const double* alphas,
                     const double* means, const double* __restrict__ const q,
                     const double* __restrict__ const l, const double* x,
                     gmm::Wishart wishart, double* err, double* alphas_d,
                     double* means_d, double* q_d, double* l_d) {
  const double CONSTANT = -n * d * 0.5 * log(2 * M_PI);
  const int    l_sz     = d * (d - 1) / 2;

  std::vector<double> Qdiags(d * k);
  std::vector<double> sum_qs(k);

  for (int i = 0; i < d * k; i++) {
    Qdiags[i] = exp(q[i]);
  }
  for (int i = 0; i < k; i++) {
    for (int j = 0; j < d; j++) {
      sum_qs[i] += q[i * d + j];
    }
  }

  std::fill(alphas_d, alphas_d + k, 0.0);
  std::fill(means_d, means_d + k * d, 0.0);
  std::fill(q_d, q_d + k * d, 0.0);
  std::fill(l_d, l_d + k * l_sz, 0.0);

  std::vector<double> main_term(k);
  std::vector<double> xcentered(d);
  std::vector<double> Qxcentered(d);

  std::vector<double> curr_means_d(d * k);
  std::vector<double> curr_q_d(d * k);
  std::vector<double> curr_L_d(l_sz * k);

  double slse = 0.;

  for (int ix = 0; ix < n; ix++) {
    const double* const curr_x = &x[ix * d];

#pragma omp parallel for firstprivate(xcentered, Qxcentered)
    for (int ik = 0; ik < k; ik++) {
      double* Qdiag = &Qdiags[ik * d];

      subtract(d, curr_x, &means[ik * d], xcentered.data());
      gmm::Qtimesx(d, Qdiag, &l[ik * l_sz], xcentered.data(),
                   Qxcentered.data());
      Qtransposetimesx(d, Qdiag, &l[ik * l_sz], Qxcentered.data(),
                       &curr_means_d[ik * d]);
      compute_q_inner_term(d, Qdiag, xcentered.data(), Qxcentered.data(),
                           &curr_q_d[ik * d]);
      compute_L_inner_term(d, xcentered.data(), Qxcentered.data(),
                           &curr_L_d[ik * l_sz]);
      main_term[ik] =
          alphas[ik] + sum_qs[ik] - 0.5 * sqnorm(d, Qxcentered.data());
    }
    double lsum = logsumexp_d(k, main_term.data(), main_term.data());
    ;
    slse += lsum;

#pragma omp parallel for
    for (int ik = 0; ik < k; ik++) {
      int means_off = ik * d;
      alphas_d[ik] += main_term[ik];
      for (int id = 0; id < d; id++) {
        means_d[means_off + id] += curr_means_d[means_off + id] * main_term[ik];
        q_d[ik * d + id] += curr_q_d[ik * d + id] * main_term[ik];
      }
      for (int i = 0; i < l_sz; i++) {
        l_d[ik * l_sz + i] += curr_L_d[ik * l_sz + i] * main_term[ik];
      }
    }
  }

  std::vector<double> lse_alphas_d(k);
  double              lse_alphas = logsumexp_d(k, alphas, lse_alphas_d.data());
#pragma omp parallel for
  for (int ik = 0; ik < k; ik++) {
    alphas_d[ik] -= n * lse_alphas_d[ik];
    for (int id = 0; id < d; id++) {
      q_d[ik * d + id] -= wishart.gamma * wishart.gamma * Qdiags[ik * d + id] *
                              Qdiags[ik * d + id] -
                          wishart.m;
    }
    for (int i = 0; i < l_sz; i++) {
      l_d[ik * l_sz + i] -= wishart.gamma * wishart.gamma * l[ik * l_sz + i];
    }
  }

  *err = CONSTANT + slse - n * lse_alphas;
  *err +=
      gmm::log_wishart_prior(d, k, wishart, sum_qs.data(), Qdiags.data(), l);
}
