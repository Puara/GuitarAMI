# IMU Drift Analysis Report

## Summary

This report describes the observation of drift in the calibrated IMU output from `src/main.cpp`, after running the magnetometer calibration routine from `puara/utils/magnetometerCalibration_embedded.h`.

The drift appears primarily in the yaw/heading output rather than roll or pitch. The behavior fits a typical AHRS convergence/magnetometer heading instability pattern: the IMU moves in various directions, then when held still the yaw continues to adjust for several seconds.

---

## What I understand from this drift

- The system is using a Madgwick filter to compute orientation from accelerometer, gyroscope, and magnetometer readings.
- The magnetometer calibration routine seems to be working in terms of producing magnitude-corrected magnetic vectors.
- Yaw is the most likely source of the drift, because heading depends on the magnetometer and is more susceptible to noise and distortion.
- Roll and pitch are typically more stable, since they are mostly determined by gravity via the accelerometer.
- The 5–10 second stabilization period matches the expected behavior of a sensor fusion filter converging after motion.

---

## Likely causes

1. **Magnetometer heading instability**
   - Magnetometer-based yaw is noisy.
   - Nearby metal, electronics, or poor calibration can cause heading drift.
   - Your calibration routine is a simple hard/soft iron correction and may not fully correct the real magnetic environment.

2. **AHRS convergence time**
   - The Madgwick filter may need several seconds to stabilize after movement.
   - During or after motion, it will continue adjusting heading values until the sensor settles.

3. **Yaw wrapping / Euler angle artifacts**
   - Yaw values near ±180° can appear to jump or drift suddenly.
   - This is a normal artifact of using Euler angles rather than quaternions.

4. **Gyro bias and sensor noise**
   - If the gyroscope has bias, the filter can drift during low-motion intervals.
   - The current code does not appear to perform explicit gyro bias calibration at startup.

---

## Evidence from the data

- The CSV data is being exported with OSC-style headers.
- The magnetometer vector magnitude in the calibrated CSV is essentially constant at 1.0, which indicates the calibration normalization is functioning.
- However, low gyro-rate segments still show yaw movement, meaning the fusion algorithm is correcting heading even when physical motion is minimal.
- This points to yaw/heading correction behavior rather than pure motion noise.

---

## Recommendations for solving drift

### 1. Improve magnetometer calibration

- Re-calibrate in a clean environment.
- Rotate the IMU through all axes slowly and evenly.
- Keep it away from metal objects, magnets, power supplies, USB cables, and loudspeakers.

### 2. Use quaternion output instead of YPR if possible

- Quaternions avoid gimbal lock and reduce misleading angle jumps.
- If your 3D pipeline accepts quaternion rotation, prefer that over Euler angles.

### 3. Add stationary detection

- Detect when the IMU is still using low gyro magnitude and accelerometer near 1g.
- Only trust yaw after a stationary period.
- During motion, reduce reliance on magnetometer corrections.

### 4. Tune the Madgwick filter

- The filter beta constant (`0.1`) may need adjustment.
- Lower beta reduces noisy corrections.
- Higher beta speeds convergence but can amplify disturbances.
- Test a few values and see how drift vs responsiveness changes.

### 5. Calibrate gyro bias

- Add a startup routine that averages gyro output while the device is still.
- Subtract that bias from subsequent gyro readings.
- That reduces slow integrated drift in the filter.

### 6. Consider sensor or fusion improvements

- If possible, use a sensor or fusion algorithm with built-in heading stability.
- The current path is raw LSM9DS1 + external Madgwick, which is the most drift-sensitive option.

---

## Practical takeaway

- The observed drift is very likely yaw/heading drift from magnetometer/AHRS correction, not a broken accelerometer or roll/pitch failure.
- Improve magnetometer calibration and apply stability detection or gyro bias compensation.
- If you can use quaternion-based orientation in the renderer, that will give the most robust result.

---

## Suggested next step

If needed, I can help implement a simple stationary yaw-lock in `src/main.cpp` or add a gyro bias calibration routine to reduce the drift further.
