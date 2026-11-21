#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <cmath>
#include <algorithm>

#include <dirent.h>

#if defined(_WIN32) && defined(__MINGW32__)

#include <sys/stat.h>

#endif

#include <hpdf.h>

#include "wildcards.hpp"
#include "ProgressBar.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IMAGE_MODE_CENTER 0
#define IMAGE_MODE_ID 1
#define IMAGE_MODE_SINGLE_CENTER 2
#define IMAGE_MODE_GROUP_2 3
#define IMAGE_MODE_FIT 4
#define IMAGE_MODE_COLLAGE 5
#define IMAGE_MODE_GRID 6

#define IMAGE_ORDER_COL 0
#define IMAGE_ORDER_ROW 1

struct ImageID {
  std::string path;
  float height;
  float width;
  int originalWidth;
  int originalHeight;
  unsigned char * pixels;
};

struct Options {
  bool bheight;
  bool bwidth;
  float height; // In points
  float width; // In points
  std::string dirName;
  std::string pdfName;
  int ImageMode;
  std::vector<ImageID> ids;
  bool rotate;
  bool help;
  std::string media;
  float mediaWidth;
  float mediaHeight;
  int groups;
  int rows;
  int cols;
  int order;
};

struct ImageHandler {
  int width;
  int height;
  unsigned char * pixels;
  std::string name;
  float newWidth;
  float newHeight;
};

/*
  
   bulkprint -d=dir -w=10cm or -h=10cm 

*/

std::string getFileNameExtension(std::string fileName) {
  int i = fileName.size() - 1;
  std::string suffix = "";
  while (i >= 0 && fileName.at(i) != '.') {
    suffix = fileName.at(i) + suffix;
    i--;
  }
  return suffix;
}

std::vector<std::string> returnImageNamesFromDir(std::string dirName) {
  std::vector<std::string> output;
  DIR * dir = opendir(dirName.c_str());
  if (!dir) {
    return output;
  }
  struct dirent * entry;
  while (dirName.size() && dirName.back() == '/') {
    dirName.pop_back();
  }
  while ((entry = readdir(dir)) != nullptr) {
#if !defined(_WIN32)
    if (entry->d_type == DT_REG) {
      std::string name = entry->d_name;
      std::string suffix = getFileNameExtension(name);
      if (suffix == "png" || suffix == "jpg" || suffix == "jpeg") {
        output.push_back(name);
        std::cout << "An image: " << name << std::endl; 
      } else {
        std::cout << "Not an image: " << name << std::endl;
      }
    }
#elif defined(_WIN32) && defined(__MINGW32__)
    std::string name = entry->d_name;
    struct _stat info;
    if (_stat((dirName + "/" + name).c_str(), &info) == 0) {
      if (S_ISREG(info.st_mode)) {
        std::string suffix = getFileNameExtension(name);
        if (suffix == "png" || suffix == "jpg" || suffix == "jpeg") {
          output.push_back(name);
          std::cout << "An image: " << name << std::endl; 
        } else {
          std::cout << "Not an image: " << name << std::endl;
        } 
      }
    }
#endif
  }
  closedir(dir);
  std::sort(output.begin(), output.end());
  return output;
}

std::vector<std::string> commandsToVector(int len, char ** argv) {
  std::vector<std::string> v;
  int i = 1;
  for (; i < len; i++) {
    v.push_back(argv[i]);
    std::cout << "arg: " << argv[i] << std::endl;
  }
  return v;
}

std::vector<std::string> getPropertyAndValue(std::string str) {
  std::vector<std::string> out;
  int equalsPos = -1;
  int i = 0;
  for (i = 0; i < str.size(); i++) {
    if (str.at(i) == '=') {
      equalsPos = i;
      break;
    }
  }
  if (equalsPos != -1) {
    out.push_back(str.substr(0, equalsPos));
    out.push_back(str.substr(equalsPos+1, str.size()));
  } else {
    out.push_back(str);
  }
  return out;
}

float stringToPoints(std::string str) {
  int i = str.size() - 1;
  float num = 0;
  std::string suffix;
  while(!(str.at(i) >= '0' && str.at(i) <= '9')) {
    suffix = str.at(i) + suffix;
    i--;
  }
  std::cout << "Suffix is " << suffix << std::endl;
  if (suffix == "cm") {
    num = std::stof(str.substr(0, i+1)) * 72.0f / 2.54f;
    std::cout << "cm to points " << num << std::endl;
  } else if (suffix == "in") {
    num = std::stof(str.substr(0, i+1)) * 72.0f;
    std::cout << "in to points: " << num << std::endl;
  } else if (suffix == "mm") {
    num = std::stof(str.substr(0, i+1)) * 72.0f / 25.4f;
    std::cout << "mm to points: " << num << std::endl;
  }
  return num;
}

Options getDefaultOptions() {
  Options out;
  out.ImageMode = IMAGE_MODE_CENTER;
  out.height = 0;
  out.width = 0;
  out.bheight = false;
  out.bwidth = false;
  out.dirName = ".";
  out.pdfName = "output.pdf";
  out.rotate = false;
  out.help = false;
  out.media = "A4";
  out.mediaWidth = 595.44f;
  out.mediaHeight = 841.68f;
  out.rows = 1;
  out.cols = 1;
  out.groups = 100;
  out.order = IMAGE_ORDER_COL;
  return out;
}

std::vector<std::string> getParentDirectory(std::string s) {
  std::string path = "";
  std::vector<std::string> out;
  int i = s.size() - 1;
  while (i >= 0 && s.at(i) != '/') {
    i--;
  }
  out.push_back(s.substr(0, i + 1));
  out.push_back(s.substr(i + 1, s.size()));
  return out;
}

Options getOptionsFromCommandLine(std::vector<std::string> cmdLines) {
  Options out = getDefaultOptions();
  int i;
  for (i = 0; i < cmdLines.size(); i++) {
    std::vector<std::string> prop = getPropertyAndValue(cmdLines.at(i));
    if (prop.at(0) == "-d" || prop.at(0) == "--dir") {
      out.dirName = prop.at(1);
      std::cout << "Dir name is " << prop.at(1) << std::endl; 
    } else if (prop.at(0) == "--add-dir") {
      std::vector<std::string> imageNames = returnImageNamesFromDir(prop.at(1));
      int i;
      for (i = 0; i < imageNames.size(); i++) {
        ImageID img;
        if (prop.at(1).size() && prop.at(1).at(prop.at(1).size() - 1) != '/') {
          img.path = prop.at(1) + '/' + imageNames.at(i);
          out.ids.push_back(img);
        } else {
          img.path = prop.at(1) + imageNames.at(i);
          out.ids.push_back(img);
        }
      }
    } else if (prop.at(0) == "--imgs") {
      std::vector<std::string> strs = getParentDirectory(prop.at(1));
      std::string pattern = strs.at(1);
      std::vector<std::string> imageNames = returnImageNamesFromDir(strs.at(0));
      int i;
      for (i = 0; i < imageNames.size(); i++) {
        if (!wildcards::match(imageNames.at(i), pattern)) { continue; }
        ImageID img;
        if (strs.at(0).size() && strs.at(0).at(strs.at(0).size() - 1) != '/') {
          img.path = strs.at(0) + '/' + imageNames.at(i);
          out.ids.push_back(img);
        } else {
          img.path = strs.at(0) + imageNames.at(i);
          out.ids.push_back(img);
        }
        std::cout << "Selected: " << strs.at(0) << imageNames.at(i) << std::endl;
      }
    } else if (prop.at(0) == "-o") {
      out.pdfName = prop.at(1);
    } else if (prop.at(0) == "-w" || prop.at(0) == "--width") {
      out.bwidth = true;
      out.width = stringToPoints(prop.at(1));
    } else if (prop.at(0) == "-h" || prop.at(0) == "--height") {
      out.bheight = true;
      out.height = stringToPoints(prop.at(1));
    } else if (prop.at(0) == "-m" || prop.at(0) == "--mode") {
      if (prop.at(1) == "id") {
        out.ImageMode = IMAGE_MODE_ID;
      } else if (prop.at(1) == "center") {
        out.ImageMode = IMAGE_MODE_SINGLE_CENTER;
      } else if (prop.at(1) == "2") {
        out.ImageMode = IMAGE_MODE_GROUP_2;
      } else if (prop.at(1) == "fit") {
        out.ImageMode = IMAGE_MODE_FIT;
      } else if (prop.at(1) == "collage") {
        out.ImageMode = IMAGE_MODE_COLLAGE;
      } else if (prop.at(1) == "grid") {
        out.ImageMode = IMAGE_MODE_GRID;
      }
    } if (prop.at(0) == "--group" && (out.ImageMode == IMAGE_MODE_COLLAGE || out.ImageMode == IMAGE_MODE_GRID)) {
      out.groups = std::stoi(prop.at(1));
    } else if (prop.at(0) == "-1b1" && out.ImageMode == IMAGE_MODE_ID) {
      ImageID img;
      img.path = prop.at(1);
      img.width = 72.0f;
      img.height = 72.0f;
      out.ids.push_back(img);
    } else if ((prop.at(0) == "-c" || prop.at(0) == "--img") && (out.ImageMode == IMAGE_MODE_SINGLE_CENTER || out.ImageMode == IMAGE_MODE_GROUP_2 || out.ImageMode == IMAGE_MODE_FIT || out.ImageMode == IMAGE_MODE_COLLAGE || out.ImageMode == IMAGE_MODE_GRID)) {
      ImageID img;
      img.path = prop.at(1);
      img.width = 0.0f;
      img.height = 0.0f;
      out.ids.push_back(img);
    } else if (prop.at(0) == "-2b2" && out.ImageMode == IMAGE_MODE_ID) {
      ImageID img;
      img.path = prop.at(1);
      img.width = 144.0f;
      img.height = 144.0f;
      out.ids.push_back(img);
    } else if (prop.at(0) == "-pp" && out.ImageMode == IMAGE_MODE_ID) {
      ImageID img;
      img.path = prop.at(1);
      img.width = 99.36f;
      img.height = 127.44f;
      out.ids.push_back(img);
    } else if (prop.at(0) == "--autorotate") {
      out.rotate = true;
    } else if (prop.at(0) == "--help") {
      out.help = true;
    } else if (prop.at(0) == "--media") {
      if (prop.at(1) == "A4" || prop.at(1) == "a4") {
        out.media = "A4";
        out.mediaWidth = 595.44f;
        out.mediaHeight = 841.68f;
      }
    } else if (prop.at(0) == "--rows" && out.ImageMode == IMAGE_MODE_GRID) {
      out.rows = std::stoi(prop.at(1));
    } else if (prop.at(0) == "--cols" && out.ImageMode == IMAGE_MODE_GRID) {
      out.cols = std::stoi(prop.at(1));
    } else if (prop.at(0) == "--major") {
      if (prop.at(1) == "row") {
        out.order = IMAGE_ORDER_ROW;
      } else if (prop.at(1) == "col") {
        out.order = IMAGE_ORDER_COL;
      }
    }
  }
  return out;
} 

int returnMin(int a, int b) {
  if (a < b) { return a; }
  return b;
}


void drawRotatedImageAtCenter(HPDF_Page & page, HPDF_Image & image, float pageWidth, float pageHeight, float w, float h) {
  float x = (pageWidth - w) / 2;
  float y = (pageHeight - h) / 2;

  HPDF_Page_GSave(page);

  HPDF_Page_Concat(page, 0, -1, 1, 0, x, y + h);

  HPDF_Page_DrawImage(page, image, 0, 0, h, w);

  HPDF_Page_GRestore(page);
}

std::vector<std::pair<float, float>> initGroup2(float w, float h) {
  std::vector<std::pair<float, float>> out;
  float widthHalf = w / 2.0f;
  float heightQuarter = h / 4.0f;
  out.push_back(std::make_pair(widthHalf, heightQuarter * 3));
  out.push_back(std::make_pair(widthHalf, heightQuarter));
  return out;
}

std::vector<std::pair<float, float>> initGridColMajor(float w, float h, float c, float r) {
  std::vector<std::pair<float, float>> out;
  float stepx = w / c / 2;
  float stepy = h / r / 2;
  float startx = stepx;
  float starty = stepy;
  for (int i = 0; i < c; i++) {
    starty = stepy + (stepy + stepy) * (r - 1);
    for (int j = 0; j < r; j++) {
      out.push_back(std::make_pair(startx, starty));
      std::cout << "g: " << startx << ", " << starty << std::endl;
      starty = starty - stepy - stepy;
    }
    startx += stepx + stepx;
  }
  return out;
}

std::vector<std::pair<float, float>> initGridRowMajor(float w, float h, float c, float r) {
  std::vector<std::pair<float, float>> out;
  float stepx = w / c / 2;
  float stepy = h / r / 2;
  float startx = stepx;
  float starty = stepy + (stepy + stepy) * (r - 1);
  for (int j = 0; j < r; j++) {
    startx = stepx;
    for (int i = 0; i < c; i++) {
      out.push_back(std::make_pair(startx, starty));
      std::cout << "g: " << startx << ", " << starty << std::endl;
      startx = startx + stepx + stepx;
    }
    starty = starty - stepy - stepy;
  }
  return out;
}

std::pair<float, float> getPictureCenter (float W, float H, int n, int index) {
  int cols = std::ceil(std::sqrt(n * W / H));
  int rows = (n + cols - 1) / cols;
  float cellW = W / cols;
  float cellH = H / rows;
  int col = index % cols;
  int row = index / cols;
  std::pair<float, float> out;
  out.first = (col + 0.5f) * cellW;
  out.second = H - (row + 0.5f) * cellH;
  return out;
}

void showHelp() {
  std::cout << "----------" << std::endl;
  std::cout << "Bulk Printer v. 2026-11-21" << std::endl;
  std::cout << "--help: Show this help..." << std::endl;
  std::cout << "--dir=<path>, -d=<path>: Set directory on default (Center) mode." << std::endl;
  std::cout << "--mode=<type>, -m=<type>: Set operation mode." << std::endl;
  std::cout << "  Available modes are id, center, fit, 2, collage, grid." << std::endl;
  std::cout << "    --add-dir=<path>: add folder path containing image (first level, non-depth-recursive)." << std::endl;
  std::cout << "    --imgs=<pattern>: image path containing wildcards (first level, non-depth-recursive)." << std::endl;
  std::cout << "  On grid:" << std::endl;
  std::cout << "    --cols=<num>: number of columns of images." << std::endl;
  std::cout << "    --rows=<num>: number of rows of images." << std::endl;
  std::cout << "    --group=<num>: number of of images in the grid." << std::endl;
  std::cout << "    --major=<type>: row for row-major order, col for col-major order." << std::endl;
  std::cout << "  On collage:" << std::endl;
  std::cout << "    --group=<num>: set collage items count inside paper." << std::endl;
  std::cout << "  On center, 2:" << std::endl;
  std::cout << "    --img=<path>, -c=<path>: set single image from path." << std::endl;
  std::cout << "  On id:" << std::endl;
  std::cout << "    -1b1=<path>: 1 by 1 id sized picture from path." << std::endl;
  std::cout << "    -2b2=<path>: 2 by 2 id sized picture from path." << std::endl;
  std::cout << "    -pp=<path>: passport sized picture from path." << std::endl;
  std::cout << "--height=<num>, -h=<num>: Set target height." << std::endl;
  std::cout << "--width=<num>, -w=<num>: Set target width." << std::endl;
  std::cout << "-o=<name>: Name of pdf file." << std::endl;
  std::cout << "--autorotate: Auto rotate pictures." << std::endl;
  std::cout << "--media=<type>: Set paper media size." << std::endl;
  std::cout << "----------" << std::endl;
}

int main(int argc, char * argv[]) {
  Options opt;
  std::vector<std::pair<float, float>> group2;
  float paperWidth = 0;
  float paperHeight = 0;
  //std::cout << getPropertyAndValue("-d=wow").at(1) << std::endl;
  //returnImageNamesFromDir("./src");
  if (argc < 2) {
    showHelp();
    return 1;
  } else {
    opt = getOptionsFromCommandLine(commandsToVector(argc, argv));
    paperWidth = opt.mediaWidth;
    paperHeight = opt.mediaHeight;
  }

  if (opt.help) {
    showHelp();
    return 0;
  }

  HPDF_Doc pdf = HPDF_New(nullptr, nullptr);

  HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);

  if (!pdf) {
    return 2;
  }

  std::vector<std::string> imageNames;
  std::vector<ImageHandler> images;

  group2 = initGroup2(paperWidth, paperHeight);

  switch (opt.ImageMode) {
    case IMAGE_MODE_CENTER: {
      const int total = imageNames.size();
      progresscpp::ProgressBar bar(total, 50);
      imageNames = returnImageNamesFromDir(opt.dirName);
      for (int i = 0; i < imageNames.size(); i++) {
        std::string completePath = opt.dirName + "/" + imageNames.at(i);
        ImageHandler img;
        int channels;
        img.pixels = stbi_load(completePath.c_str(), &(img.width), &(img.height), &channels, 3);
        if (!img.pixels) {
          return 3;
        }
        float scale = 1;
        if (opt.bwidth && opt.bheight) {
          img.newWidth = opt.width;
          img.newHeight = opt.height;
        } else if (opt.bwidth) {
          scale = opt.width / img.width;
          img.newWidth = opt.width;
          img.newHeight = img.height * scale;
        } else if (opt.bheight) {
          scale = opt.height / img.height;
          img.newHeight = opt.height;
          img.newWidth = img.width * scale;
        } else {
          std::cerr << "You forgot to specify target width or height!" << std::endl;
        }
        images.push_back(img);
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_ID: case IMAGE_MODE_COLLAGE: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      for (int i = 0; i < opt.ids.size(); i++) {
        int channels;
        opt.ids.at(i).pixels = stbi_load(opt.ids.at(i).path.c_str(), &(opt.ids.at(i).originalWidth), &(opt.ids.at(i).originalHeight), &channels, 3);
        if (!opt.ids.at(i).pixels) {
          return 3;
        }
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_SINGLE_CENTER: case IMAGE_MODE_GROUP_2: case IMAGE_MODE_GRID: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      for (int i = 0; i < opt.ids.size(); i++) {
        int channels;
        opt.ids.at(i).pixels = stbi_load(opt.ids.at(i).path.c_str(), &(opt.ids.at(i).originalWidth), &(opt.ids.at(i).originalHeight), &channels, 3);
        if (!opt.ids.at(i).pixels) {
          return 3;
        }
        if (opt.bwidth && opt.bheight) {
          opt.ids.at(i).width = opt.width;
          opt.ids.at(i).height = opt.height;
        } else if (opt.bwidth) {
          float scale = opt.width / opt.ids.at(i).originalWidth;
          opt.ids.at(i).width = opt.width;
          opt.ids.at(i).height = opt.ids.at(i).originalHeight * scale;
        } else if (opt.bheight) {
          float scale = opt.height / opt.ids.at(i).originalHeight;
          opt.ids.at(i).height = opt.height;
          opt.ids.at(i).width = opt.ids.at(i).originalWidth * scale;
        } else {
          std::cerr << "You forgot to specify target width or height!" << std::endl;
        }
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_FIT: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      for (int i = 0; i < opt.ids.size(); i++) {
        int channels;
        opt.ids.at(i).pixels = stbi_load(opt.ids.at(i).path.c_str(), &(opt.ids.at(i).originalWidth), &(opt.ids.at(i).originalHeight), &channels, 3);
        if (!opt.ids.at(i).pixels) {
          return 3;
        }
          
        float testScale = paperWidth / opt.ids.at(i).originalWidth;
        float newHeight = opt.ids.at(i).originalHeight * testScale;

        if (newHeight <= paperHeight) {
          if (opt.rotate && opt.ids.at(i).originalWidth > opt.ids.at(i).originalHeight) {
            float scale = paperHeight / opt.ids.at(i).originalWidth;
            opt.ids.at(i).width = paperHeight;
            opt.ids.at(i).height = opt.ids.at(i).originalHeight * scale;
          } else {
            float scale = paperWidth / opt.ids.at(i).originalWidth;
            opt.ids.at(i).width = paperWidth;
            opt.ids.at(i).height = opt.ids.at(i).originalHeight * scale;
          }
        } else {
          float scale = paperHeight / opt.ids.at(i).originalHeight;
          opt.ids.at(i).height = paperHeight;
          opt.ids.at(i).width = opt.ids.at(i).originalWidth * scale;
        }
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
  }


  switch (opt.ImageMode) {
    case IMAGE_MODE_CENTER: {
      const int total = imageNames.size();
      progresscpp::ProgressBar bar(total, 50);
      for (int i = 0; i < imageNames.size(); i++) {
        float x = (paperWidth - images.at(i).newWidth) / 2;
        float y = (paperHeight - images.at(i).newHeight) / 2;
        HPDF_Page page = HPDF_AddPage(pdf);

        HPDF_Page_SetWidth(page, paperWidth);
        HPDF_Page_SetHeight(page, paperHeight);

        HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, images.at(i).pixels, images.at(i).width, images.at(i).height, HPDF_CS_DEVICE_RGB, 8);
        if (opt.rotate && images.at(i).newWidth > images.at(i).newHeight) {
          drawRotatedImageAtCenter(page, image, paperWidth, paperHeight, images.at(i).newHeight, images.at(i).newWidth);
        } else {
          HPDF_Page_DrawImage(page, image, x, y, images.at(i).newWidth, images.at(i).newHeight);
        }
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_ID: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      float x_start = 20, y_start = paperHeight - 25;
      HPDF_Page page = HPDF_AddPage(pdf);
      HPDF_Page_SetWidth(page, paperWidth);
      HPDF_Page_SetHeight(page, paperHeight);
      for (int i = 0; i < opt.ids.size(); i++) {
        y_start -= opt.ids.at(i).height;
        if (y_start <= 25) {
          page = HPDF_AddPage(pdf);
          HPDF_Page_SetWidth(page, paperWidth);
          HPDF_Page_SetHeight(page, paperHeight);
          x_start = 20;
          y_start = paperHeight - 25 - opt.ids.at(i).height;
        }
        HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, opt.ids.at(i).pixels, opt.ids.at(i).originalWidth, opt.ids.at(i).originalHeight, HPDF_CS_DEVICE_RGB, 8);
        if (opt.ids.at(i).height == 72.0f) {
          for (int j = 0; j < 6; j++) {
            HPDF_Page_DrawImage(page, image, x_start, y_start, opt.ids.at(i).width, opt.ids.at(i).height);
            x_start += 20 + 72.0f;
          }
          y_start -= 25;
          x_start = 20;
        } else if (opt.ids.at(i).height == 144.0f) {
          for (int j = 0; j < 3; j++) {
            HPDF_Page_DrawImage(page, image, x_start, y_start, opt.ids.at(i).width, opt.ids.at(i).height);
            x_start += 40.0f + 144.0f;
          }
          y_start -= 25;
          x_start = 20;
        } else if (opt.ids.at(i).height == 127.44f) {
          for (int j = 0; j < 4; j++) {
            HPDF_Page_DrawImage(page, image, x_start, y_start, opt.ids.at(i).width, opt.ids.at(i).height);
            x_start += 35.0f + 99.36f;
          }
          y_start -= 25;
          x_start = 20;
        }

        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_SINGLE_CENTER: case IMAGE_MODE_FIT: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      for (int i = 0; i < opt.ids.size(); i++) {
        float x = (paperWidth - opt.ids.at(i).width) / 2;
        float y = (paperHeight - opt.ids.at(i).height) / 2;

        HPDF_Page page = HPDF_AddPage(pdf);

        HPDF_Page_SetWidth(page, paperWidth);
        HPDF_Page_SetHeight(page, paperHeight);

        ImageID img = opt.ids.at(i);

        HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, img.pixels, img.originalWidth, img.originalHeight, HPDF_CS_DEVICE_RGB, 8);

        if (opt.rotate && img.width > img.height) {
          drawRotatedImageAtCenter(page, image, paperWidth, paperHeight, img.height, img.width);
        } else {
          HPDF_Page_DrawImage(page, image, x, y, img.width, img.height);
        }
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_GROUP_2: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      int max_pos = 2;
      int j = 0;
      HPDF_Page page = HPDF_AddPage(pdf);
      HPDF_Page_SetWidth(page, paperWidth);
      HPDF_Page_SetHeight(page, paperHeight);
      for (int i = 0; i < opt.ids.size(); i++) {
        if (j == max_pos) {
          page = HPDF_AddPage(pdf);
          HPDF_Page_SetWidth(page, paperWidth);
          HPDF_Page_SetHeight(page, paperHeight);
          j = 0;
        }
        const ImageID & img = opt.ids.at(i);
        const std::pair<float, float> &pos = group2.at(i);

        HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, img.pixels, img.originalWidth, img.originalHeight, HPDF_CS_DEVICE_RGB, 8);
        HPDF_Page_DrawImage(page, image, pos.first - img.width / 2.0f, pos.second - img.height / 2.0f, img.width, img.height);
        j++;
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_COLLAGE: {
      const int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      int max_pos = opt.groups;
      int j = 0;
      HPDF_Page page = HPDF_AddPage(pdf);
      HPDF_Page_SetWidth(page, paperWidth);
      HPDF_Page_SetHeight(page, paperHeight);
      for (int i = 0; i < opt.ids.size(); i++) {
        if (j == max_pos) {
          page = HPDF_AddPage(pdf);
          HPDF_Page_SetWidth(page, paperWidth);
          HPDF_Page_SetHeight(page, paperHeight);
          j = 0;
        }
        const ImageID & img = opt.ids.at(i);
        const std::pair<float, float> pos = getPictureCenter(paperWidth, paperHeight, max_pos, j);

        HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, img.pixels, img.originalWidth, img.originalHeight, HPDF_CS_DEVICE_RGB, 8);

        if (opt.bwidth && opt.bheight) {
          opt.ids.at(i).width = opt.width;
          opt.ids.at(i).height = opt.height;
        } else if (opt.bwidth) {
          float scale = opt.width / opt.ids.at(i).originalWidth;
          opt.ids.at(i).width = opt.width;
          opt.ids.at(i).height = opt.ids.at(i).originalHeight * scale;
        } else if (opt.bheight) {
          float scale = opt.height / opt.ids.at(i).originalHeight;
          opt.ids.at(i).height = opt.height;
          opt.ids.at(i).width = opt.ids.at(i).originalWidth * scale;
        } else {
          std::cerr << "You forgot to specify target width or height!" << std::endl;
        }

        HPDF_Page_DrawImage(page, image, pos.first - img.width / 2.0f, pos.second - img.height / 2.0f, img.width, img.height);
        j++;
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
    case IMAGE_MODE_GRID: {
      std::vector<std::pair<float, float>> coords;
      int imageCount = returnMin(opt.groups, opt.cols * opt.rows);
      int j = 0;
      int total = opt.ids.size();
      progresscpp::ProgressBar bar(total, 50);
      HPDF_Page page = HPDF_AddPage(pdf);
      HPDF_Page_SetWidth(page, paperWidth);
      HPDF_Page_SetHeight(page, paperHeight);

      if (opt.order == IMAGE_ORDER_ROW) {
        coords = initGridRowMajor(paperWidth, paperHeight, opt.cols, opt.rows);
      } else if (opt.order == IMAGE_ORDER_COL) {
        coords = initGridColMajor(paperWidth, paperHeight, opt.cols, opt.rows);
      }

      for (int i = 0; i < opt.ids.size(); i++) {
        if (j == imageCount) {
          page = HPDF_AddPage(pdf);
          HPDF_Page_SetWidth(page, paperWidth);
          HPDF_Page_SetHeight(page, paperHeight);
          j = 0;
        }
        
        const ImageID & img = opt.ids.at(i);
        HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, img.pixels, img.originalWidth, img.originalHeight, HPDF_CS_DEVICE_RGB, 8);
        
        HPDF_Page_DrawImage(page, image, coords.at(j).first - img.width / 2.0f, coords.at(j).second - img.height / 2.0f, img.width, img.height);
        j++;
        ++bar;
        bar.display();
      }
      bar.done();
      break;
    }
  }

  std::cout << "Saving..." << std::endl;
  if (images.size() || opt.ids.size()) {
    HPDF_SaveToFile(pdf, opt.pdfName.c_str());
    HPDF_Free(pdf);
  } else {
    std::cout << "Nothing to save, no pictures detected or used..." << std::endl;
  }
  switch (opt.ImageMode) {
    case IMAGE_MODE_CENTER: {
      for (int i = 0; i < images.size(); i++) {
        stbi_image_free(images.at(i).pixels);
      }
      break;
    }
    case IMAGE_MODE_ID: case IMAGE_MODE_SINGLE_CENTER: case IMAGE_MODE_GROUP_2: case IMAGE_MODE_FIT: case IMAGE_MODE_COLLAGE: case IMAGE_MODE_GRID: {
      for (int i = 0; i < opt.ids.size(); i++) {
        stbi_image_free(opt.ids.at(i).pixels);
      }
      break;
    }
  }
  return 0;
}
