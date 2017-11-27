#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
struct Layer {
  std::string name;
  std::vector<size_t> weightShape;
  std::vector<uint16_t> weights;
  size_t nBiases;
  std::vector<uint16_t> biases;
  Layer(const std::string& name, const std::string& wShape, const std::string& wStr, const std::string& nBS, const std::string& bStr):
    name(name) 
  {  
    size_t nWeights(1);
    std::istringstream wsISS(wShape);
    while (wsISS.good()) {
      int i;
      wsISS>>i;
      if (wsISS.rdbuf()->in_avail()>0) {
	nWeights *= i;
	weightShape.push_back(i);
      }
    }

    std::istringstream wISS(wStr);
    while (wISS.good()) {
      float d;
      wISS>>d;    //FIXME file should contain ints
      if (wISS.rdbuf()->in_avail()>0) weights.push_back(d);
    }
    assert(nWeights==weights.size());

    std::istringstream nISS(nBS);
    nISS >> nBiases;

    std::istringstream bISS(bStr);
    while (bISS.good()) {
      float d;
      bISS>>d;    //FIXME file should contain ints
      if (bISS.rdbuf()->in_avail()>0) biases.push_back(d);
    }
    assert(nBiases==biases.size());
  }
};

typedef std::vector<Layer> Layers_t;

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
