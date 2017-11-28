#include "TrainedLayers.hpp"
void readLayersFlatFile(const std::string& path, Layers_t& layers) {
  std::cout << "readLayersFlatFile " << path << std::endl;
  std::ifstream file;
  file.open(path, std::ios::in);
  
  if (!file) {
    std::cerr << "Error opening file at " << path << std::endl;
    return;
  }
  
  while (file.good()) {
    std::string name, wshape, weights, nBias, biases;
    std::getline(file, name);
    if (!file.good()) break;
    std::getline(file, wshape);
    if (!file.good()) break;
    std::getline(file, weights);
    if (!file.good()) break;
    std::getline(file, nBias);
    if (!file.good()) break;
    std::getline(file, biases);
    if (!file.good()) break;
    Layer l(name, wshape, weights, nBias, biases);
    layers.push_back(l);
    //read separator
    std::string emptyLine;
    std::getline(file, emptyLine);
    if (!file.good() || !emptyLine.empty() ) break;
  }
  return;
}
