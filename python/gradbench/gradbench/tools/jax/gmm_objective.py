import math

import jax.numpy as jnp
from jax.scipy.special import logsumexp, multigammaln


def log_wishart_prior(*, k, p, gamma, m, sum_qs, Qdiags, l):
    n = p + m + 1

    out = jnp.sum(
        0.5 * gamma * gamma * (jnp.sum(Qdiags**2, axis=1) + jnp.sum(l**2, axis=1))
        - m * sum_qs
    )

    C = n * p * (jnp.log(gamma / math.sqrt(2)))
    return -out + k * (C - multigammaln(0.5 * n, p))


def gmm_objective(*, d, k, n, x, m, gamma, alpha, mu, q, l):
    Qdiags = jnp.exp(q)
    sum_qs = jnp.sum(q, axis=1)

    cols = jnp.repeat(
        jnp.arange(d - 1),
        jnp.arange(d - 1, 0, -1),
        total_repeat_length=d * (d - 1) // 2,
    )
    rows = jnp.concatenate([jnp.arange(c + 1, d) for c in range(d - 1)])
    Ls = jnp.zeros((k, d, d), dtype=l.dtype).at[:, rows, cols].set(l)

    xcentered = x[:, None, :] - mu[None, ...]
    Lxcentered = Qdiags * xcentered + jnp.einsum("ijk,mik->mij", Ls, xcentered)
    sqsum_Lxcentered = jnp.sum(Lxcentered**2, axis=2)
    inner_term = alpha + sum_qs - 0.5 * sqsum_Lxcentered
    slse = jnp.sum(logsumexp(inner_term, axis=1))

    CONSTANT = -n * d * 0.5 * math.log(2 * math.pi)
    return (
        CONSTANT
        + slse
        - n * logsumexp(alpha, axis=0)
        + log_wishart_prior(
            k=k, p=d, gamma=gamma, m=m, sum_qs=sum_qs, Qdiags=Qdiags, l=l
        )
    )
