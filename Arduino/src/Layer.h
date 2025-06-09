// Layer.h
#ifndef LAYER_H
#define LAYER_H

#include <Arduino.h>

class LayerManager {
public:
  LayerManager();
  void begin(int maxConfiguredLayers);
  bool setCurrentLayer(int layer);
  int getCurrentLayer() const;
  int getMaxLayers() const;

private:
  int currentLayer;
  int maxLayers;
};

extern LayerManager layerManager;

#endif