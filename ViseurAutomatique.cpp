#include "ViseurAutomatique.h"
#include <Arduino.h>

ViseurAutomatique::ViseurAutomatique(int p1, int p2, int p3, int p4, float& distanceRef)
  : _stepper(AccelStepper::HALF4WIRE, p1, p3, p2, p4), _distance(distanceRef) {
  _stepper.setMaxSpeed(1000);
  _stepper.setAcceleration(300);
  _stepper.setSpeed(300);
  _stepper.setCurrentPosition(_angleEnSteps(_angleMin)); 
}

void ViseurAutomatique::update() {
  _currentTime = millis();

  switch (_etat) {
    case INACTIF:
      _inactifState(_currentTime);
      break;
    case SUIVI:
      _suiviState(_currentTime);
      break;
    case REPOS:
      _reposState(_currentTime);
      break;
  }

  _stepper.run(); 
}

// --- États FSM ---

void ViseurAutomatique::_inactifState(unsigned long cT) {
  // Moteur à l'arrêt
  _stepper.stop();
  // Aucun mouvement
}

void ViseurAutomatique::_suiviState(unsigned long cT) {
  if (_distance < _distanceMinSuivi || _distance > _distanceMaxSuivi) {
    _etat = REPOS;
    return;
  }

  
  float angleSuivi = map(_distance, _distanceMinSuivi, _distanceMaxSuivi, _angleMax, _angleMin);
  long cible = _angleEnSteps(angleSuivi);
  _stepper.moveTo(cible);
}

void ViseurAutomatique::_reposState(unsigned long cT) {
  if (_distance >= _distanceMinSuivi && _distance <= _distanceMaxSuivi) {
    _etat = SUIVI;
    return;
  }

  
  long positionRepos = _angleEnSteps(_angleMin); 
  _stepper.moveTo(positionRepos);
}

// --- Configuration ---

void ViseurAutomatique::activer() {
  _etat = REPOS;
}

void ViseurAutomatique::desactiver() {
  _etat = INACTIF;
  _stepper.stop();
}

void ViseurAutomatique::setAngleMin(float angle) {
  _angleMin = angle;
}

void ViseurAutomatique::setAngleMax(float angle) {
  _angleMax = angle;
}

void ViseurAutomatique::setPasParTour(int steps) {
  _stepsPerRev = steps;
}

void ViseurAutomatique::setDistanceMinSuivi(float distanceMin) {
  _distanceMinSuivi = distanceMin;
}

void ViseurAutomatique::setDistanceMaxSuivi(float distanceMax) {
  _distanceMaxSuivi = distanceMax;
}

// --- Lecture ---

float ViseurAutomatique::getAngle() const {
  return (float)_stepper.currentPosition() * 360.0 / _stepsPerRev;
}

const char* ViseurAutomatique::getEtatTexte() const {
  switch (_etat) {
    case INACTIF: return "Inactif";
    case SUIVI:   return "Suivi";
    case REPOS:   return "Repos";
    default:      return "Inconnu";
  }
}


long ViseurAutomatique::_angleEnSteps(float angle) const {
  return angle * _stepsPerRev / 360.0;
}
