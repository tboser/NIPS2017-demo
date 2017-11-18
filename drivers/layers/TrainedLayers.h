/* this is C++ */
#include <string>
#include <vector>
struct Layer {
  std::string name;
  std::array<size_t, 4> weights_shape;
  std::vector<int8_t> weights;
  size_t bias_shape;
  std::vector<int8_t> biases;
};

typedef std::vector<Layer> Layers_t;

void readLayersFlatFile(const std::string& path, Layers_t layers) {
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file) {
        std::cout << "Error opening file at " << path << std::endl;
        return {};
    }

}
