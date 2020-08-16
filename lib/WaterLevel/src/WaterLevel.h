#ifndef __WATERLEVEL_H__
#define __WATERLEVEL_H__

#include <ADCLib.h>

class WaterLevelSensor {
public:
  static constexpr double DEFAULT_LEVEL_R = 680e3;  // 680KOhm
  static constexpr double DEFAULT_REF_R = 2e6;  // 2MOhm
  static constexpr double DEFAULT_WATER_R = 0.5e6;  // 500KOhm == 0.5MOhm; determined experimentally (stick multimeter probes in water)

  // For a given nLevels, the sensor should have a ladder with nLevels resistors,
  // and _nLevels+1 wires in contact with the water.
  //
  // levelValues should have length of nLevels.
  // The first element, levelValues[0], should represent "level too low" 
  // (i.e., at most one wire, the lowest one, is immersed).
  WaterLevelSensor(
    ADCPort& adcPort, int nLevels, const double *levelValues,
    double levelR = DEFAULT_LEVEL_R, double refR = DEFAULT_REF_R,
    double waterR = DEFAULT_WATER_R
  ) :
    _adcPort(adcPort), _nLevels(nLevels), _levelValues(levelValues),
    _levelR(levelR), _refR(refR), _waterR(waterR),
    _levelAdcThresholds(new double[nLevels-1]) {

    _initAdcThresholds();
  }
  ~WaterLevelSensor() {
    delete _levelAdcThresholds;
   }

  double level();

private:

  ADCPort& _adcPort;
  const int _nLevels;
  const double* const _levelValues;
  const double _levelR;  // Resistor value between level wires
  const double _refR;    // Resistor value to ground (from sensor output)
  const double _waterR;  // Resistance of water between two immersed wires

  double* const _levelAdcThresholds;

  void _initAdcThresholds();  // Calculate ADC thresholds based on resistor values
};

#endif  /* __WATERLEVEL_H__ */