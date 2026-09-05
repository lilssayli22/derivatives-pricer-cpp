# Pricer Monte-Carlo

Prix d'options par Monte-Carlo, modèle de Black-Scholes multidimensionnel.

## Classes

- `Option` — le produit. `payoff` transforme une trajectoire en montant.
- `BlackScholesModel` — la dynamique. `asset` produit une trajectoire.
- `MonteCarlo` — la méthode numérique. Connaît les deux autres, qui s'ignorent.

## Compilation

```bash
mkdir build && cd build
cmake .. && make
```
