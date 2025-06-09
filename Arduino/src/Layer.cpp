// Layer.cpp
#include "Layer.h"
#include "Config.h" // To potentially get max layers if not passed

LayerManager::LayerManager() : currentLayer(1), maxLayers(1) {}

void LayerManager::begin(int maxConfiguredLayers) {
    maxLayers = (maxConfiguredLayers > 0) ? maxConfiguredLayers : 1;
    currentLayer = 1; // Always start at layer 1
    Serial.print("LayerManager initialized. Max layers: ");
    Serial.println(maxLayers);
}

bool LayerManager::setCurrentLayer(int layer) {
  if (layer > 0 && layer <= maxLayers) {
    currentLayer = layer;
    Serial.print("Layer changed to: ");
    Serial.println(currentLayer);
    return true;
  }
  Serial.print("Invalid layer requested: ");
  Serial.println(layer);
  return false;
}

int LayerManager::getCurrentLayer() const {
  return currentLayer;
}

int LayerManager::getMaxLayers() const {
    return maxLayers;
}

LayerManager layerManager; // Global instance