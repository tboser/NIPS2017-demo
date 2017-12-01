#ifndef TRAINEDLAYERS_HPP
#define TRAINEDLAYERS_HPP 1
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
struct Layer {
  std::string name;
  std::vector<size_t> weightShape;
  std::vector<int16_t> weights;
  size_t nBiases;
  std::vector<int16_t> biases;
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

void readLayersFlatFile(const std::string& path, Layers_t& layers);
#endif
