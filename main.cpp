#include "CPNG.h"

#include <iostream>
#include <sstream>
#include <filesystem>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main(int inArgC, char** inpArgV)
{
  int retVal = 1;
  if (inArgC != 3)
  {
    std::cout<<"Syntax: "<<inpArgV[0]<<" pngFile \"New order\""<<std::endl;
    std::cout<<"Ex: "<<inpArgV[0]<<" ./pngToReorder.png \"2 0 1 3\""<<std::endl;
  }
  else
  {
    std::filesystem::path imgFile(inpArgV[1]);
    CPNG pngFile;
    if (pngFile.load_from_PNG(imgFile))
    {
      std::cout<<"After load"<<std::endl;
      pngFile.dump_chunks(std::cout);

      std::vector<std::size_t> newOrder;
      auto iss = std::istringstream{inpArgV[2]};
      auto id = std::size_t{};
      while (iss >> id)
      {
        newOrder.push_back(id);
      }
      
      std::cout<<"New order: ";
      for (auto val:newOrder) {std::cout<<val<<" ";}
      std::cout<<std::endl;

      pngFile.reorder_data_chunks(newOrder);
     
      std::cout<<"After reorder"<<std::endl;
      pngFile.dump_chunks(std::cout); 

      std::filesystem::path outFile((imgFile.parent_path().string().empty()? "":imgFile.parent_path().string()+"/")+imgFile.stem().string()+"_reordered"+imgFile.extension().string());

      if (pngFile.save_to_PNG(outFile))
      {
        std::cout<<"Reordered png saved in: "<<outFile<<std::endl;
        retVal = 0;
      }
      else
      {
        std::cout<<"Cannot save: '"<<outFile<<"'"<<std::endl;
      }
    }
    else
    {
      std::cout<<"PNG NOT OK"<<std::endl;
    }
  }
  
  return retVal;
} // main
