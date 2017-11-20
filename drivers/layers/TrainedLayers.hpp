#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
struct Layer {
  std::string name;
  std::vector<size_t> weightShape;
  std::vector<int8_t> weights;
  size_t nBiases;
  std::vector<int8_t> biases;
  Layer(const std::string& name, const std::string& wShape, const std::string& wStr, int nBias, const std::string& bStr):
    name(name), nBiases(nBias) 
  {  
    std::istringstream wsISS(wShape);
    while (wsISS.good()) {
      int i;
      wsISS>>i;
      weightShape.push_back(i);
    }

    std::istringstream wISS(wStr);
    while (wISS.good()) {
      float d;
      wISS>>d;    //FIXME file should contain ints
      weights.push_back(d);
    }

    std::istringstream bISS(bStr);
    while (bISS.good()) {
      float d;
      bISS>>d;    //FIXME file should contain ints
      biases.push_back(d);
    }
  }
};

typedef std::vector<Layer> Layers_t;

void readLayersFlatFile(const std::string& path, Layers_t& layers) {
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file) {
        std::cerr << "Error opening file at " << path << std::endl;
        return;
    }

    while (file.good()) {
      std::string name, wshape, weights, biases;
      int nBias;
      std::getline(file, name);
      if (!file.good()) break;
      std::getline(file, wshape);
      if (!file.good()) break;
      std::getline(file, weights);
      if (!file.good()) break;
      file>>nBias;
      if (!file.good()) break;
      std::getline(file, biases);
      if (!file.good()) break;
      layers.push_back(Layer(name, wshape, weights, nBias, biases));
      //read separator
      std::string emptyLine;
      std::getline(file, emptyLine);
      if (!file.good() || !emptyLine.empty() ) break;
    }
    return;
}
