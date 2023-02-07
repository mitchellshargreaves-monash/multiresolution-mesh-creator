#include "destripe.hpp"
#include "pngutils.hpp"


int main(int argc, char **argv) {

    std::vector<char *> noa;    // all non-option arguments
    bool debug = false;  // product debug images
    for(int i=1; i<argc; i++) {
        if (argv[i][0] != '-')
            noa.push_back(argv[i]);
        else if (strcasecmp(argv[i], "-d") == 0)
            debug = true;
        else {
            printf("Unknown option %s\n", argv[i]);
            return 42;
        }
    }
    if (noa.size() < 2) {
        printf("Usage:  Destripe [-d] <input file> <output file>\n");
        return 42;
    }
    int w, h;
    uint8 *image = read_8bit_png_file(noa[0], w, h);
    printf("opened '%s', w=%d h=%d\n", noa[0], w, h);

    std::vector<int> seam{-1, 2066,4684,7574,10385,13222,15937,18531,21198,23826,26506,29175,31772, int(w)};


    size_t YC = size_t(h)/1000;
    auto output = destripe(image, size_t(w), size_t(h), YC, seam);

    printf("Output file is '%s', %d x %d\n", noa[1], w, h);
    write_8bit_png_file(noa[1], &output[0], w, h);
}
