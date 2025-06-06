# Gaussian Mixture Model Fitting (GMM)

This eval is adapted from "Objective GMM: Gaussian Mixture Model Fitting" from
section 4.1 of the [ADBench paper][], with a more complete specification and
incorporating fixes for three errors in the original paper and
[implementations][adbench]:

1. There was a sign error on exactly one of either the likelihood or the prior.
2. The precision matrices were incorrectly transposed.
3. The Wishart prior is over the precision matrices, not the covariances.

It computes the [gradient][] of the [logarithm][] of the [posterior
probability][] for a [multivariate Gaussian][] [mixture model][] (GMM) with a
[Wishart][] [prior][] on the [precision matrices][]. It defines a module named
`gmm`, which consists of two functions `objective` and `jacobian`, both of which
take the same input:

```typescript
import type { Float, Int, Runs } from "gradbench";

/** Independent variables for which the gradient must be computed. */
interface Independent {
  /** Parametrization for weights. */
  alpha: Float[];

  /** Means. */
  mu: Float[][];

  /** Logarithms of diagonal part for constructing precision matrices. */
  q: Float[][];

  /** Strictly lower triangular part for constructing precision matrices. */
  l: Float[][];
}

/** The full input. */
interface Input extends Runs, Independent {
  /** Dimension of the space. */
  d: Int;

  /** Number of means. */
  k: Int;

  /** Number of points. */
  n: Int;

  /** Data points. */
  x: Float[][];

  /** Additional degrees of freedom in Wishart prior. */
  m: Int;

  /** Precision in Wishart prior. */
  gamma: Float;
}

export namespace gmm {
  /** Compute the log posterior probability. */
  function objective(input: Input): Float;

  /** Compute the gradient of the log posterior probability. */
  function jacobian(input: Input): Independent;
}
```

## Definition

First, some standard definitions: we denote the probability density function for
the $`k`$-dimensional multivariate Gaussian distribution as

```math
f_{\mathcal{N}_k(\boldsymbol{\mu}, \boldsymbol{\Sigma})}(\boldsymbol{x}) = \frac{\exp(-\frac{1}{2}(\boldsymbol{x} - \boldsymbol{\mu})^\top \mathbf{\Sigma}^{-1} (\boldsymbol{x} - \boldsymbol{\mu}))}{\sqrt{(2\pi)^k \det(\mathbf{\Sigma})}}.
```

and the probability density function for the $`p`$-dimensional Wishart
distribution as

```math
f_{W_p(\mathbf{V}, n)}(\mathbf{X}) = \frac{\det(\mathbf{X})^{(n-p-1)/2} \exp(-\frac{1}{2} \text{tr}(\mathbf{V}^{-1} \mathbf{X}))}{2^{(np)/2} \det(\mathbf{V})^{n/2} \Gamma_p(\frac{n}{2})}
```

where $`\text{tr}`$ is the [trace][] and $`\Gamma_p`$ is the [multivariate gamma
function][].

---

The `Input` field `k` is the number of mixture components $`K \in \mathbb{N}`$,
and `n` is the number of observations $`N \in \mathbb{N}`$. The mixture weights
$`\boldsymbol{\phi} \in [0, 1]^K`$ are computed from `alpha` representing
$`\boldsymbol{\alpha} \in \mathbb{R}^K`$, via the formula

```math
\phi_k = \frac{\exp(\alpha_k)}{\sum_{k'=1}^K \exp(\alpha_{k'})}
```

which ensures that $`\sum_{k=1}^K \phi_k = 1`$. These represent the probability
for each mixture component, so conceptually there is a vector
$`\boldsymbol{z} \in \{1, \dots, K\}^N`$ where $`z_i`$ is the component of
observation $`i`$; but $`\boldsymbol{z}`$ are [latent variables][] and do not
appear in the computation.

Because the model is multivariate, the observations `x` are a [row-major][]
encoding of the matrix $`\mathbf{X} \in \mathbb{R}^{N \times D}`$ where `d` is
the dimension $`D \in \mathbb{N}`$ of the space. Because the distribution is
Gaussian, `mu` is a row-major encoding of the matrix
$`\mathbf{M} \in \mathbb{R}^{K \times D}`$ giving the mean
$`\boldsymbol{\mu}_k \in \mathbb{R}^D`$ for the component with index
$`k \in \{1, \dots, K\}`$.

Conceptually, that component also has a [positive-definite][] [covariance
matrix][] $`\mathbf{\Sigma}_k \in \mathbb{R}^{D \times D}`$. However, the
covariance matrix is not directly used in the computation; only its inverse, the
precision matrix, is used. The `q` and `l` fields parametrize these precision
matrices; their elements at index $`k`$ are the vectors
$`\boldsymbol{q}_k \in \mathbb{R}^D`$ and
$`\boldsymbol{l}_k \in \mathbb{R}^{\frac{D(D-1)}{2}}`$, respectively. From
these, we construct the [lower triangular][] matrix

```math
Q(\boldsymbol{q}_k, \boldsymbol{l}_k) = \begin{bmatrix}
  \exp(q_{k,1}) & 0 & \cdots & 0 \\
  l_{k,1} & \exp(q_{k,2}) & \cdots & 0 \\
  \vdots & \vdots & \ddots & \vdots \\
  l_{k,D-1} & l_{k,D-1+D-2} & \cdots & \exp(q_{k,D})
\end{bmatrix} \in \mathbb{R}^{D \times D}
```

by exponentiating each value of $`\boldsymbol{q}_k`$ to form the diagonal and
using $`\boldsymbol{l}_k`$ to fill the rest, column by column. Then we use this
to compute the precision matrix as
$`\mathbf{\Sigma}_k^{-1} = Q(\boldsymbol{q}_k, \boldsymbol{l}_k)^\top Q(\boldsymbol{q}_k, \boldsymbol{l}_k)`$.

We will refer to the collection of all the means and covariance matrices
together by the symbol $`\boldsymbol{\theta}`$. Then, since component $`k`$ has
probability $`\phi_k`$ and distribution
$`\mathcal{N}_D(\boldsymbol{\mu}_k, \boldsymbol{\Sigma}_k)`$, we can write the
likelihood by multiplying across all observations and summing across all mixture
components, to get the GMM probability density function

```math
p(\mathbf{X} \mid \boldsymbol{\theta}, \boldsymbol{\phi}) = \prod_{i=1}^N \sum_{k=1}^K \phi_k f_{\mathcal{N}_D(\boldsymbol{\mu}_k, \boldsymbol{\Sigma}_k)}(\boldsymbol{x}_i).
```

Finally, the fields `m` and `gamma` are $`m \in \mathbb{Z}_{\geq 0}`$ and
$`\gamma > 0`$ respectively, with which we parametrize a Wishart distribution on
the precision matrices via
$`\mathbf{V} = \frac{1}{\gamma^2} I \in \mathbb{R}^{D \times D}`$ and
$`n = D + m + 1`$ to define our prior as

```math
p(\boldsymbol{\theta}) = \prod_{k=1}^K f_{W_D(\mathbf{V}, n)}(\mathbf{\Sigma}_k^{-1})
```

so the overall `objective` function computes the log posterior

```math
F_\mathbf{X}(\boldsymbol{\theta}, \boldsymbol{\phi}) = \log(p(\mathbf{X} \mid \boldsymbol{\theta}, \boldsymbol{\phi}) p(\boldsymbol{\theta}))
```

and the `jacobian` function computes $`\nabla F_{\mathbf{X}}`$ with respect to
the `Independent` variables that parametrize $`\boldsymbol{\theta}`$ and
$`\boldsymbol{\phi}`$.

## Implementation

To actually compute `objective`, it is typical to first perform some algebraic
simplifications, since the Gaussian and Wishart probability density functions
include determinants and matrix inversions that would be expensive to compute
naively. By observing that

```math
\det(\mathbf{\Sigma}_k^{-1}) = \det(Q(\boldsymbol{q}_k, \boldsymbol{L}_k))^2 = \prod_{j=1}^D \exp(q_{k,j})^2 = \exp\Bigg(2\sum_{j=1}^D q_{k,j}\Bigg)
```

and $`\det(\mathbf{\Sigma}_k) = \det(\mathbf{\Sigma}_k^{-1})^{-1}`$, we can
define

```math
\beta_{i,k} = \alpha_k -\frac{1}{2} \|Q(\boldsymbol{q}_k, \boldsymbol{l}_k)(\boldsymbol{x}_i - \boldsymbol{\mu}_k)\|^2 + \sum_{j=1}^D q_{k,j}
```

and write

```math
p(\mathbf{X} \mid \boldsymbol{\theta}, \boldsymbol{\phi}) = \prod_{i=1}^N \frac{1}{\sqrt{(2\pi)^D} \sum_{k=1}^K \exp(\alpha_k)} \sum_{k=1}^K \exp(\beta_{i,k}).
```

For the prior we have
$`\text{tr}(\mathbf{V}^{-1} \mathbf{\Sigma}_k^{-1}) = \gamma^2 \|Q(\boldsymbol{q}_k, \boldsymbol{l}_k)\|_F^2`$
and $`\det(\mathbf{V})^{n/2} = \gamma^{-nD}`$, so

```math
f_{W_D(\mathbf{V}, n)}(\mathbf{\Sigma}_k) = \bigg(\frac{\gamma}{\sqrt{2}}\bigg)^{nD} \frac{1}{\Gamma_D(\frac{n}{2})} \exp\Bigg({-\frac{\gamma^2}{2} \|Q(\boldsymbol{q}_k, \boldsymbol{l}_k)\|_F^2 + m \sum_{j=1}^D q_{k,j}}\Bigg).
```

Then we can define a [helper function][`lse`]
$`\text{logsumexp} : \mathbb{R}^K \to \mathbb{R}`$ which we compute stably by
first computing the maximum of the vector as

```math
\text{logsumexp}(\boldsymbol{v}) = \log \sum_{k=1}^K \exp(v_k) = \max \boldsymbol{v} + \log \sum_{k=1}^K \exp(v_k - \max \boldsymbol{v}),
```

and finally use
$`F_\mathbf{X}(\boldsymbol{\theta}, \boldsymbol{\phi}) = \log p(\mathbf{X} \mid \boldsymbol{\theta}, \boldsymbol{\phi}) + \log p(\boldsymbol{\theta})`$
to push the logarithms down, yielding

```math
\log p(\mathbf{X} \mid \boldsymbol{\theta}, \boldsymbol{\phi}) = -N\bigg(\frac{D}{2} \log 2\pi + \text{logsumexp}(\boldsymbol{\alpha})\bigg) + \sum_{i=1}^N \text{logsumexp}(\boldsymbol{\beta}_i)
```

and

```math
\log p(\boldsymbol{\theta}) = K\bigg(nD \log \frac{\gamma}{\sqrt{2}} - \log \Gamma_D\Big(\frac{n}{2}\Big)\bigg) - \frac{\gamma^2}{2} \|Q(\boldsymbol{q}_k, \boldsymbol{l}_k)\|_F^2 + m \sum_{k=1}^K \sum_{j=1}^D q_{k,j}.
```

Note that you can use the `--validate-naive` flag on this eval to switch the
validator for the `objective` function (not `jacobian`) to check against a more
direct implementation of the definition for this eval, instead of the default
which uses these algebraic simplifications. This is only recommended for small
problem instances, due to numerical issues with the naive implementation.

## Commentary

This eval is straightforward to implement from an AD perspective, as it computes
a dense gradient of a scalar-valued function. The objective function can be
easily expressed in a scalar way (see the [C++ implementation][] or the [Futhark
implementation][]), or through linear algebra operations (see the [PyTorch
implementation][]). An even simpler eval with the same properties is [`llsq`][],
which you might consider implementing first. After implementing `gmm`,
implementing [`lstm`][] or [`ode`][] should not be so difficult.

### Parallel execution

This eval is straightforward to parallelise. The [C++ implementation][] has been
parallelised with OpenMP.

[adbench]: https://github.com/microsoft/ADBench
[adbench paper]: https://arxiv.org/abs/1807.10129
[covariance matrix]: https://en.wikipedia.org/wiki/Covariance_matrix
[c++ implementation]: /cpp/gradbench/evals/gmm.hpp
[futhark implementation]: /tool/futhark/gmm.fut
[gradient]: https://en.wikipedia.org/wiki/Gradient
[latent variables]:
  https://en.wikipedia.org/wiki/Latent_and_observable_variables
[logarithm]: https://en.wikipedia.org/wiki/Logarithm
[lower triangular]: https://en.wikipedia.org/wiki/Triangular_matrix
[mixture model]: https://en.wikipedia.org/wiki/Mixture_model
[multivariate gamma function]:
  https://en.m.wikipedia.org/wiki/Multivariate_gamma_function
[multivariate gaussian]:
  https://en.wikipedia.org/wiki/Multivariate_normal_distribution
[positive-definite]: https://en.wikipedia.org/wiki/Definite_matrix
[posterior probability]: https://en.wikipedia.org/wiki/Posterior_probability
[precision matrices]: https://en.wikipedia.org/wiki/Precision_(statistics)
[prior]: https://en.wikipedia.org/wiki/Prior_probability
[pytorch implementation]:
  /python/gradbench/gradbench/tools/pytorch/gmm_objective.py
[row-major]: https://en.wikipedia.org/wiki/Row-_and_column-major_order
[trace]: https://en.m.wikipedia.org/wiki/Trace_(linear_algebra)
[wishart]: https://en.m.wikipedia.org/wiki/Wishart_distribution
[`llsq`]: /evals/llsq
[`lse`]: /evals/lse
[`lstm`]: /evals/lstm
[`ode`]: /evals/ode
