# Magnetometer Calibration Recommendations

## Summary
This document summarizes the recommended approach for magnetometer calibration on ESP32 embedded hardware versus desktop environments.

## Problem
The current `puara_gestures::utils::Calibration::generateMagnetometerMatrices()` implementation uses Eigen dynamic matrix operations and solves an ellipsoid fit.

On desktop, this may work fine, but on the ESP32 the same code can trigger a `Stack canary watchpoint` failure in `loopTask` during Eigen matrix computations.

## Recommendations

### 1. Keep the full Eigen calibration for desktop
- If the algorithm is validated on desktop and produces good calibration, keep that implementation for desktop use.
- Desktop environments have much more stack and heap than the ESP32.

### 2. Use an embedded-safe path for ESP32
Create a separate calibration path for ESP32 that avoids large dynamic Eigen workloads in the loop task.

#### Suggested changes
- Use a fixed-size sample buffer rather than unbounded `std::vector` growth.
- Reduce sample count to a safe number, such as `256` or `512` samples.
- Prefer `float` instead of `double` in the embedded calibration algorithm.
- Avoid dynamic `Eigen::MatrixXd` allocations during critical runtime.
- Move heavy calibration computation out of the main loop task into a dedicated FreeRTOS task with a larger stack.

### 3. Add an embedded-friendly API
Provide an API that accepts raw pointers and counts instead of taking `std::vector` by value.

Example:
```cpp
int generateMagnetometerMatrices(const Coord3D* samples, size_t count);
```

This avoids unnecessary copying and allows the caller to manage a fixed array or preallocated buffer.

### 4. Use fixed-size storage in firmware
In `src/main.cpp`, store calibration samples in a fixed-size array:

```cpp
static const size_t maxCalibrationSamples = 512;
static std::array<puara_gestures::Coord3D, maxCalibrationSamples> calibrationRawMagData;
static size_t calibrationRawMagCount = 0;
```

Collect samples directly into that array.

### 5. Consider a simpler embedded calibration algorithm
If full ellipsoid fit is too heavy, use a simpler algorithm on ESP32:
- hard-iron correction using min/max for each axis
- optionally compute scale factors for soft-iron compensation
- or a PCA-based calibration

This is much cheaper and more likely to be stable on embedded hardware.

## Implementation strategy

### Conditional build
Use compile-time selection to keep desktop and embedded implementations separate:

```cpp
#ifdef ESP32
int generateMagnetometerMatrices(const Coord3D* samples, size_t count) {
    return generateMagnetometerMatricesEmbedded(samples, count);
}
#else
int generateMagnetometerMatrices(const std::vector<Coord3D>& data) {
    return generateMagnetometerMatricesDesktop(data);
}
#endif
```

### Dedicated calibration task
If the heavier method is still required, run the calibration on a separate FreeRTOS task:
- allocate a larger stack for that task
- execute the Eigen-based fit there
- send the result back to the main loop

## Practical next steps
1. Implement fixed-size sample collection in firmware.
2. Add a pointer/count API for calibration data.
3. Use `float` and fixed-size Eigen matrices for embedded calibration.
4. If needed, move the heavy computation to a dedicated FreeRTOS task.
5. Keep the desktop Eigen implementation for non-embedded builds.

## Notes
- 5000 samples of 3D doubles is about 120KB of storage, but the bigger issue is the computation stack and heap used by Eigen.
- A bounded sample count and an embedded-safe computation path are the best way to make calibration reliable on ESP32.
