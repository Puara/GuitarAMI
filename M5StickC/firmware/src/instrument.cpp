// Some high-level gestural descriptors of the GuitarAMI
// using the IMU
// Edu Meneses - 2020 (IDMIL)

#include "instrument.h"

void Instrument::updateInstrumentIMU (float gyroX, float gyroY, float gyroZ) {

    Instrument::gyroXArray.push_back(gyroX);
    Instrument::gyroYArray.push_back(gyroY);
    Instrument::gyroZArray.push_back(gyroZ);
    if(Instrument::gyroXArray.size() > Instrument::queueAmount) {
        Instrument::gyroXArray.pop_front();
        Instrument::gyroYArray.pop_front();
        Instrument::gyroZArray.pop_front();
    }

    std::deque<float>::iterator minX = std::min_element(Instrument::gyroXArray.begin(), Instrument::gyroXArray.end());
    std::deque<float>::iterator maxX = std::max_element(Instrument::gyroXArray.begin(), Instrument::gyroXArray.end());
    std::deque<float>::iterator minY = std::min_element(Instrument::gyroYArray.begin(), Instrument::gyroYArray.end());
    std::deque<float>::iterator maxY = std::max_element(Instrument::gyroYArray.begin(), Instrument::gyroYArray.end());
    std::deque<float>::iterator minZ = std::min_element(Instrument::gyroZArray.begin(), Instrument::gyroZArray.end());
    std::deque<float>::iterator maxZ = std::max_element(Instrument::gyroZArray.begin(), Instrument::gyroZArray.end());

    float gyroAbsX = std::abs(gyroX);
    float gyroAbsY = std::abs(gyroY);
    float gyroAbsZ = std::abs(gyroZ);
    
    // Instrument shake
    if (gyroAbsX > shakeThreshold) {
       Instrument::shakeX = leakyIntegrator(gyroAbsX/maxGyroValue, Instrument::shakeX, 0.6, Instrument::leakyShakeFreq, Instrument::leakyShakeTimerX);
    } else {
        Instrument::shakeX = leakyIntegrator(0, Instrument::shakeX, 0.3, Instrument::leakyShakeFreq, Instrument::leakyShakeTimerX);
        if (Instrument::shakeX < shakeMinThreshold) {
            Instrument::shakeX = 0;
        }
    }
    if (gyroAbsY > shakeThreshold) {
        Instrument::shakeY = leakyIntegrator(gyroAbsY/maxGyroValue, Instrument::shakeY, 0.6, Instrument::leakyShakeFreq, Instrument::leakyShakeTimerY);
    } else {
        Instrument::shakeY = leakyIntegrator(0, Instrument::shakeY, 0.3, Instrument::leakyShakeFreq, Instrument::leakyShakeTimerY);
        if (Instrument::shakeY < shakeMinThreshold) {
            Instrument::shakeY = 0;
        }
    }
    if (gyroAbsZ > shakeThreshold) {
        Instrument::shakeZ = leakyIntegrator(gyroAbsZ/maxGyroValue, Instrument::shakeZ, 0.6, Instrument::leakyShakeFreq, Instrument::leakyShakeTimerZ);
    } else {
        Instrument::shakeZ = leakyIntegrator(0, Instrument::shakeZ, 0.3, Instrument::leakyShakeFreq, Instrument::leakyShakeTimerZ);
        if (Instrument::shakeZ < shakeMinThreshold) {
            Instrument::shakeZ = 0;
        }
    }

    // Instrument jab
    if (*maxX-*minX > Instrument::jabThreshold) {
      Instrument::jabX = *maxX + *minX;
    }
    if (*maxY-*minY > Instrument::jabThreshold) {
      Instrument::jabY = *maxY + *minY;
    }
    if (*maxZ-*minZ > Instrument::jabThreshold) {
      Instrument::jabZ = *maxZ + *minZ;
    }
}

// Simple leaky integrator implementation
// Create a unsigned long global variable for time counter for each leak implementation (timer)

float Instrument::leakyIntegrator (float reading, float old_value, float leak, int frequency, unsigned long& timer) {
  
  float new_value;
  if (frequency <= 0) {
    new_value = reading + (old_value * leak);
  } else if (millis() - (1000 / frequency) < timer) {  
    new_value = reading + old_value;
  } else {
    new_value = reading + (old_value * leak);
    timer = millis();
  }
  return new_value;
}

float Instrument::getShakeX() {
  return Instrument::shakeX;
};

float Instrument::getShakeY() {
  return Instrument::shakeY;
};

float Instrument::getShakeZ() {
  return Instrument::shakeZ;
};

float Instrument::getJabX() {
  return Instrument::jabX;
};

float Instrument::getJabY() {
  return Instrument::jabY;
};

float Instrument::getJabZ() {
  return Instrument::jabZ;
};
