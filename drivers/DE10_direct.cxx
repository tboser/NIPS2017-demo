#include <iostream>
#include <vector>
#include "mnist/mnist_reader.hpp"
#include "layers/TrainedLayers.hpp"
#include "FPGAIORegs.hpp"
#define MNIST_DATA_LOCATION "../mnist"
#define KERAS_PARMS "../flat_weights.txt"

using namespace std;

static FPGAIORegs s_fpgaIO("/tmp/mmap.bin");

void sendMatricesFPGA(const std::string& path) {
  cout << "sendMatricesFPGA" <<endl;
  Layers_t layers;
  readLayersFlatFile(path, layers);

  //LeNet C1 5x5 Convs with one input channel and 6 filters
  s_fpgaIO.writeCnvLayer(layers[0], 0);
  
  //LeNet C2 5x5 Convs with 6 input channels and 16 filters
  s_fpgaIO.writeCnvLayer(layers[1], 1);

  //LeNet FC1 256*120, with 16 rows per FPGA module
  s_fpgaIO.writeFCLayer(layers[2], 2, 16);

  //LeNet FC1 120*84, with 16 rows per FPGA module
  s_fpgaIO.writeFCLayer(layers[3], 3);

  //LeNet FC1 256*120, with 16 rows per FPGA module
  s_fpgaIO.writeFCLayer(layers[4], 4);
}

void startForwardPass(const ImageBatch_t& imgBatch) {
  cout << "startForwardPass" << endl;
  s_fpgaIO.writeImgBatch(imgBatch);
}

void waitOnFPGA() {
  cout << "waitOnFPGA" <<endl;
}

void readPredictions(Results_t& preds) {
  cout << "readPredictions" <<endl;
  waitOnFPGA();
  Results_t batchPred;
  ///....
  preds.insert(preds.end(), batchPred.begin(), batchPred.end());
}

void processResults(const Results_t& /*res*/) {
  cout << "processResults" << endl;
}





int main(int argc, char* argv[]) {
  std::cout << "Terasic DE10 MNIST demo driver starting" << std::endl;

  //Read keras parms, prepare matrices
  sendMatricesFPGA(KERAS_PARMS);

  // MNIST_DATA_LOCATION set by Makefile
  std::cout << "MNIST data directory: " << MNIST_DATA_LOCATION << std::endl;
  
  // Load MNIST data
  mnist::MNIST_dataset<std::vector, std::vector<uint8_t>, uint8_t> dataset =
    mnist::read_dataset<std::vector, std::vector, uint8_t, uint8_t>(MNIST_DATA_LOCATION);
  
  std::cout << "Nbr of training images = " << dataset.training_images.size() << std::endl;
  std::cout << "Nbr of training labels = " << dataset.training_labels.size() << std::endl;
  std::cout << "Nbr of test images = " << dataset.test_images.size() << std::endl;
  std::cout << "Nbr of test labels = " << dataset.test_labels.size() << std::endl;

  //  unsigned int nTestImgs(dataset.test_images.size());
  /// one prediction per img

  //Loop over mnist images stride X
  Results_t mnistPred;
  //mnistPred.reserve(nTestImgs);
  unsigned int i(0);
  const int STRIDE(2);
  //FIXME!!!!!!!!!!!!!!!  while (i<nTestImgs) {
  while (i<100) {
    ImageBatch_t imgBatch;
    imgBatch.reserve(STRIDE);
    for (int j=0; j<STRIDE; ++j) {
      Image_t img;
      img.reserve(dataset.test_images[i].size());
      for (uint8_t pixel: dataset.test_images[i]) img.push_back((uint16_t)pixel); 
      imgBatch.push_back(img);
      ++i;
    }
    startForwardPass(imgBatch);
    readPredictions(mnistPred);
  }
  processResults(mnistPred);
    
  cout << "Terasic DE10 MNIST demo driver ending" << endl;
  return 0;
}
