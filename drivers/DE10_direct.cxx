#include <iostream>
#include <vector>
#include "mnist/mnist_reader.hpp"
#define MNIST_DATA_LOCATION "/home/calaf/heptrkx/AI-HEP.Trk/nips_demo/drivers/mnist"

using namespace std;
typedef std::vector<uint8_t> Image_t;
typedef std::vector<uint8_t> Results_t;

void startForwardPass(const std::vector<Image_t> /*imgBatch*/) {
  cout << "startForwardPass" << endl;
}
void waitOnFPGA() {
  cout << "waitOnFPGA" <<endl;
}

void readPredictions(Results_t preds) {
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

  // MNIST_DATA_LOCATION set by MNIST cmake config
  std::cout << "MNIST data directory: " << MNIST_DATA_LOCATION << std::endl;
  
  // Load MNIST data
  mnist::MNIST_dataset<std::vector, std::vector<uint8_t>, uint8_t> dataset =
    mnist::read_dataset<std::vector, std::vector, uint8_t, uint8_t>(MNIST_DATA_LOCATION);
  
  std::cout << "Nbr of training images = " << dataset.training_images.size() << std::endl;
  std::cout << "Nbr of training labels = " << dataset.training_labels.size() << std::endl;
  std::cout << "Nbr of test images = " << dataset.test_images.size() << std::endl;
  std::cout << "Nbr of test labels = " << dataset.test_labels.size() << std::endl;

  unsigned int nTestImgs(dataset.test_images.size());
  /// one prediction per img
  //Read keras parms, prepare matrices
  //Send_matrices_FPGA(...)

  //Loop over mnist images stride X
  Results_t mnistPred;
  //mnistPred.reserve(nTestImgs);
  int i(0);
  const int STRIDE(10);
  while (i<nTestImgs) {
    std::vector<Image_t> imgBatch;
    imgBatch.reserve(STRIDE);
    for (int j=0; j<STRIDE; ++j) imgBatch.push_back(dataset.test_images[i++]);
    startForwardPass(imgBatch);
    readPredictions(mnistPred);
  }
  processResults(mnistPred);
    

  cout << "Terasic DE10 MNIST demo driver ending" << endl;
  return 0;
}
