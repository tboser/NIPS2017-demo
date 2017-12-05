#include <iostream>
#include <vector>
#include <unistd.h>
#include "mnist/mnist_reader.hpp"
#include "TrainedLayers.hpp"
#include "FPGAIORegs.hpp"
#define MNIST_DATA_LOCATION "../mnist"
#define KERAS_PARMS "../flat_weights_discrete.txt"

using namespace std;

void sendMatricesFPGA(const std::string& path, const FPGAIORegs& fpgaIO) {
  cout << "sendMatricesFPGA" <<endl;
  Layers_t layers;
  readLayersFlatFile(path, layers);

  //LeNet C1 5x5 Convs with one input channel and 6 filters
  fpgaIO.writeCnvLayer(layers[0], 0);
  
  //LeNet C2 5x5 Convs with 6 input channels and 16 filters
  fpgaIO.writeCnvLayer(layers[1], 1);

  //LeNet FC1 256*120, with 16 rows per FPGA module
  fpgaIO.writeFCLayer(layers[2], 2, 16);

  //LeNet FC1 120*84
  fpgaIO.writeFCLayer(layers[3], 3);

  //LeNet FC1 84*10
  fpgaIO.writeFCLayer(layers[4], 4);
}

void startForwardPass(const ImageBatch_t& imgBatch, const FPGAIORegs& fpgaIO) {
  cout << "startForwardPass" << endl;
  fpgaIO.writeImgBatch(imgBatch);
  //signal to the FPGA it is time to process the batch
  fpgaIO.startImgProc();
}

int waitOnFPGA(const FPGAIORegs& fpgaIO) {
  cout << "waitOnFPGA starts" <<endl;
  int waitMus = fpgaIO.waitOnImgProc();
  usleep(1000);
  cout << "waitOnFPGA waited " << waitMus <<"mus" <<endl;
  return waitMus;
}

void readPredictions(Results_t& preds, const FPGAIORegs& fpgaIO) {
  cout << "readPredictions" <<endl;
  int waitMus=waitOnFPGA(fpgaIO);
  Results_t batchPred = {Result_t(), Result_t()};
  fpgaIO.readResults(batchPred);
  ///....
  preds.insert(preds.end(), batchPred.begin(), batchPred.end());
}

template <class LABELS>
void processResults(const Results_t& res, const LABELS& labels ) {
  cout << "processResults" << endl;
  int iImg(0);
  for (auto imgRes : res) {
    std::cout << std::dec << iImg << ':';
    std::cout << std::dec << (int)labels[iImg++] << ':';
    for (auto prob : imgRes) std::cout << prob << ' ';
    std::cout << std::endl;
  }
}





int main(int argc, char* argv[]) {
  std::cout << "Terasic DE10 MNIST demo driver starting" << std::endl;
  int selectStep(13);
  if (argc == 2) {
    selectStep=atoi(argv[1]);
  }

#ifdef ONDE10
  FPGAIORegs fpgaIO("/dev/mem",3,256);
#else
  FPGAIORegs fpgaIO("/tmp/simde10.bin",3);
#endif  
  
  //Read keras parms, prepare matrices
  sendMatricesFPGA(KERAS_PARMS, fpgaIO);

  // MNIST_DATA_LOCATION set by Makefile
  std::cout << "MNIST data directory: " << MNIST_DATA_LOCATION << std::endl;
  
  // Load MNIST data
  mnist::MNIST_dataset<std::vector, std::vector<uint8_t>, uint8_t> dataset =
    mnist::read_dataset<std::vector, std::vector, uint8_t, uint8_t>(MNIST_DATA_LOCATION);
  
  std::cout << "Nbr of training images = " << dataset.training_images.size() << std::endl;
  std::cout << "Nbr of training labels = " << dataset.training_labels.size() << std::endl;
  std::cout << "Nbr of test images = " << dataset.test_images.size() << std::endl;
  std::cout << "Nbr of test labels = " << dataset.test_labels.size() << std::endl;

  fpgaIO.resetImgProc();
  //fpgaIO.selectOutput(); //read last layer
  fpgaIO.selectOutput(selectStep);

  //Loop over mnist images stride X
  Results_t mnistPred;
  mnistPred.reserve(dataset.test_images.size());
  unsigned int i(0);
  const int STRIDE(2);
  // while (i<dataset.test_images.size()) {
  while (i<4) {
    ImageBatch_t imgBatch;
    imgBatch.reserve(STRIDE);
    for (int j=0; j<STRIDE; ++j) {
      Image_t img;
      img.reserve(dataset.test_images[i].size());
      for (uint8_t pixel: dataset.test_images[i]) img.push_back((uint16_t)pixel); 
      imgBatch.push_back(img);
      ++i;
    }
    startForwardPass(imgBatch, fpgaIO);
    readPredictions(mnistPred, fpgaIO);
  }
  processResults(mnistPred, dataset.test_labels);
    
  cout << "Terasic DE10 MNIST demo driver ending" << endl;
  return 0;
}
