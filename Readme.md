# EECS583 Final Project

Partial reimplementation of the algorithm in paper "Vectorizing programs with IF‑statements for processors with SIMD extensions", 2019.

## Functionality test

```bash
cd tests/testing
./run.sh basic.t        # Basic maskedload IR test
./run.sh basic2.t       # Multiple maskedload IR test
./runc.sh masked_load   # Test masked load C
./runc.sh masked_store  # Test masked store C
```

## Benchmark

```bash
cd tests/bench
./run.sh benchmark1
./run.sh benchmark2
./run.sh benchmark3
```

This will run both scalarized version and the vectorized version and also check the correctness of them.
